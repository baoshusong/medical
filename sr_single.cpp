// 单图超分 (严格对齐 model/onnx.py + read_ima_image + save_as_ima)
//   用法: sr_single <input.IMA> <out_prefix> [model.onnx]
//   预处理: HU -> clip(-1000,400) -> (hu+1000)/1400*255 -> uint8 -> /255 -> (x-0.5)/0.5 -> [-1,1]
//   后处理: pred*0.5+0.5 -> clip(0,1) -> *255 -> uint8
//   输出: <out>_4x.dcm (8-bit, slope=5.5556 inter=-1000) + <out>_4x.bmp + <out>_lr.bmp
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcuid.h>
#include "sr/OrtShim.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
#include <chrono>

static double getSlope(DcmDataset *d){ Float64 v; return d->findAndGetFloat64(DCM_RescaleSlope,v).good()?double(v):1.0; }
static double getInter(DcmDataset *d){ Float64 v; return d->findAndGetFloat64(DCM_RescaleIntercept,v).good()?double(v):0.0; }
static std::string getS(DcmDataset *d, const DcmTagKey &k){
    const char *v=nullptr; if(d->findAndGetString(k,v).good() && v) return std::string(v); return {};
}

static bool writeBmp8(const char *path, const unsigned char *px, int W, int H)
{
    FILE *f=fopen(path,"wb"); if(!f) return false;
    const int rowBytes=(W+3)&~3, dataSz=rowBytes*H, hdrSz=14+40+1024, fsz=hdrSz+dataSz;
    auto p16=[&](int v){ unsigned char b[2]={(unsigned char)(v&0xff),(unsigned char)((v>>8)&0xff)}; fwrite(b,1,2,f); };
    auto p32=[&](int v){ unsigned char b[4]={(unsigned char)(v&0xff),(unsigned char)((v>>8)&0xff),(unsigned char)((v>>16)&0xff),(unsigned char)((v>>24)&0xff)}; fwrite(b,1,4,f); };
    fwrite("BM",1,2,f); p32(fsz); p16(0); p16(0); p32(hdrSz);
    p32(40); p32(W); p32(H); p16(1); p16(8); p32(0); p32(dataSz); p32(2835); p32(2835); p32(256); p32(0);
    for(int i=0;i<256;++i){ unsigned char b[4]={(unsigned char)i,(unsigned char)i,(unsigned char)i,0}; fwrite(b,1,4,f); }
    std::vector<unsigned char> row(rowBytes,0);
    for(int y=H-1;y>=0;--y){ memcpy(row.data(), px+size_t(y)*W, W); fwrite(row.data(),1,rowBytes,f); }
    fclose(f); return true;
}

int main(int argc,char*argv[])
{
    const char *inPath=argc>1?argv[1]:"model/test.IMA";
    const char *outPre=argc>2?argv[2]:"model/test";
    const char *mdlPath=argc>3?argv[3]:"model/swinir_med_4x.onnx";

    DcmFileFormat ff;
    if(ff.loadFile(inPath).bad()){ printf("load fail: %s\n",inPath); return 1; }
    DcmDataset *d=ff.getDataset();
    Uint16 R=0,C=0; d->findAndGetUint16(DCM_Rows,R); d->findAndGetUint16(DCM_Columns,C);
    const int W0=(int)C, H0=(int)R;
    const double slope=getSlope(d), inter=getInter(d);
    const Uint16 *raw=nullptr; unsigned long cnt=0;
    if(!d->findAndGetUint16Array(DCM_PixelData,raw,&cnt).good()||cnt<static_cast<unsigned long>(W0*H0)){ printf("pixeldata fail\n"); return 1; }
    float wc=0,ww=400; { Float64 c,w; if(d->findAndGetFloat64(DCM_WindowCenter,c).good()) wc=float(c); if(d->findAndGetFloat64(DCM_WindowWidth,w).good()) ww=float(w); }
    std::string spacingStr=getS(d,DCM_PixelSpacing), winCStr=getS(d,DCM_WindowCenter), winWStr=getS(d,DCM_WindowWidth);
    std::string patName=getS(d,DCM_PatientName), patId=getS(d,DCM_PatientID);
    printf("input: %s  %dx%d  slope=%g inter=%g  WC=%g WW=%g\n", inPath, W0, H0, slope, inter, wc, ww);

    // 预处理: HU -> clip(-1000,400) -> (hu+1000)/1400*255 -> uint8 -> /255 -> (x-0.5)/0.5 -> [-1,1]
    std::vector<unsigned char> inU8(size_t(W0*H0),0);
    std::vector<float> xin(size_t(W0*H0),0.0f);
    for(size_t i=0;i<xin.size();++i){
        double hu=double(raw[i])*slope+inter;
        hu=std::clamp(hu,-1000.0,400.0);
        long v=std::lround((hu+1000.0)/1400.0*255.0);
        unsigned char u8=(unsigned char)std::clamp(v,0L,255L);
        inU8[i]=u8;
        float x=u8/255.0f;
        xin[i]=(x-0.5f)/0.5f;
    }
    char inbmp[1024]; snprintf(inbmp,sizeof(inbmp),"%s_lr.bmp",outPre);
    writeBmp8(inbmp, inU8.data(), W0, H0);
    printf("wrote %s (输入 %dx%d)\n", inbmp, W0, H0);

    Ort::Env env(ORT_LOGGING_LEVEL_WARNING,"sr_single");
    Ort::SessionOptions opt; opt.SetIntraOpNumThreads(4); opt.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    std::string mp(mdlPath); std::wstring wpath(mp.begin(),mp.end());
    Ort::Session sess(env,wpath.c_str(),opt);
    Ort::AllocatorWithDefaultOptions a;
    auto inN=sess.GetInputNameAllocated(0,a); auto outN=sess.GetOutputNameAllocated(0,a);
    int64_t sh[4]={1,1,H0,W0};
    Ort::MemoryInfo mem=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
    auto inT=Ort::Value::CreateTensor<float>(mem,xin.data(),xin.size(),sh,4);
    const char* iN[1]={inN.get()}; const char* oN[1]={outN.get()};
    auto t0=std::chrono::steady_clock::now();
    auto outs=sess.Run(Ort::RunOptions{nullptr},iN,&inT,1,oN,1);
    auto t1=std::chrono::steady_clock::now();
    const float* hr=outs[0].GetTensorData<float>();
    auto os=outs[0].GetTensorTypeAndShapeInfo().GetShape();
    int HH=os.size()>=4?(int)os[2]:H0*4, HW=os.size()>=4?(int)os[3]:W0*4;
    printf("ONNX out: %dx%d  infer %.0f ms\n", HH, HW, std::chrono::duration<double,std::milli>(t1-t0).count());

    // 后处理: pred*0.5+0.5 -> clip(0,1) -> *255 -> uint8
    std::vector<unsigned char> outU8(size_t(HH*HW),0);
    for(size_t i=0;i<outU8.size();++i){
        float v=std::clamp(hr[i]*0.5f+0.5f,0.0f,1.0f);
        outU8[i]=(unsigned char)std::lround(v*255.0f);
    }
    char bmp[1024]; snprintf(bmp,sizeof(bmp),"%s_4x.bmp",outPre);
    writeBmp8(bmp, outU8.data(), HW, HH);
    printf("wrote %s (输出 %dx%d)\n", bmp, HW, HH);

    // DICOM (对齐 save_as_ima): 8-bit, slope=5.5556, inter=-1000
    d->putAndInsertString(DCM_SOPClassUID, UID_CTImageStorage);
    d->putAndInsertString(DCM_SOPInstanceUID, "1.2.826.0.1.3680043.8.498.sr4x");
    d->putAndInsertUint16(DCM_Rows,(Uint16)HH);
    d->putAndInsertUint16(DCM_Columns,(Uint16)HW);
    d->putAndInsertUint16(DCM_SamplesPerPixel,1);
    d->putAndInsertString(DCM_PhotometricInterpretation,"MONOCHROME2");
    d->putAndInsertUint16(DCM_BitsAllocated,8);
    d->putAndInsertUint16(DCM_BitsStored,8);
    d->putAndInsertUint16(DCM_HighBit,7);
    d->putAndInsertUint16(DCM_PixelRepresentation,0);
    d->putAndInsertString(DCM_RescaleSlope,"5.5556");
    d->putAndInsertString(DCM_RescaleIntercept,"-1000");
    if(!spacingStr.empty()) d->putAndInsertString(DCM_PixelSpacing, spacingStr.c_str());
    if(!winCStr.empty()) d->putAndInsertString(DCM_WindowCenter, winCStr.c_str());
    if(!winWStr.empty()) d->putAndInsertString(DCM_WindowWidth, winWStr.c_str());
    if(!patName.empty()) d->putAndInsertString(DCM_PatientName, patName.c_str());
    if(!patId.empty()) d->putAndInsertString(DCM_PatientID, patId.c_str());
    d->putAndInsertUint8Array(DCM_PixelData, outU8.data(), (Uint32)outU8.size());
    char dcmpath[1024]; snprintf(dcmpath,sizeof(dcmpath),"%s_4x.dcm",outPre);
    if(ff.saveFile(dcmpath, EXS_LittleEndianExplicit).good()) printf("wrote %s\n", dcmpath);
    else printf("DICOM write FAIL\n");
    return 0;
}
