/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <list>
#include <map>
#include <mutex>
#include "AXFrame.hpp"
#include "BaseLinkage.h"
#include "BaseSensor.h"
#include "IModule.h"
#include "IObserver.h"

namespace AX_PANO {

#define MAX_SENSOR_COUNT (2)
#define MAX_PIPE_PER_DEV (3)

typedef struct _PIPE_FRAMERATE_INFO_T {
    AX_U8 nPipeID;
    AX_F32 fPipeFramerate;

    _PIPE_FRAMERATE_INFO_T(AX_U8 nPipe, AX_F32 fFramerate) {
        nPipeID = nPipe;
        fPipeFramerate = fFramerate;
    }
} PIPE_FRAMERATE_INFO_T;

typedef struct _RAW_DISPATCH_THREAD_PARAM {
    AX_U8 nSnsID;
    AX_U8 nDevID;
    AX_F32 fDevFramerate;
    vector<PIPE_FRAMERATE_INFO_T> vecTargetPipeFramerate;
    AX_SNS_HDR_MODE_E eHdrMode;
    AX_BOOL bThreadRunning;
    std::thread hThread;

    _RAW_DISPATCH_THREAD_PARAM() {
        nDevID = 0;
        fDevFramerate = 0;
        vecTargetPipeFramerate.clear();
        eHdrMode = AX_SNS_LINEAR_MODE;
        bThreadRunning = AX_FALSE;
    }
} RAW_DISPATCH_THREAD_PARAM_T, *RAW_DISPATCH_THREAD_PARAM_PTR;

typedef struct _YUV_THREAD_PARAM {
    AX_U8 nSnsID;
    AX_U8 nPipeID;
    AX_U8 nIspChn;
    AX_BOOL bMultiplex;
    AX_BOOL bThreadRunning;
    std::thread hThread;

    _YUV_THREAD_PARAM() {
        nPipeID = 0;
        nIspChn = 0;
        bMultiplex = AX_FALSE;
        bThreadRunning = AX_FALSE;
    }
} YUV_THREAD_PARAM_T, *YUV_THREAD_PARAM_PTR;

class CSensorMgr : public IModule, public IFrameRelease {
public:
    CSensorMgr() = default;
    virtual ~CSensorMgr() = default;

public:
    virtual AX_BOOL Init() override;
    virtual AX_BOOL DeInit() override;
    virtual AX_BOOL Start() override;
    virtual AX_BOOL Stop() override;

    virtual AX_VOID VideoFrameRelease(CAXFrame* pFrame) override;

    AX_BOOL Start(CBaseSensor* pSensor);
    AX_BOOL Stop(CBaseSensor* pSensor);
    CBaseSensor* GetSnsInstance(AX_U32 nIndex);
    AX_U32 GetSensorCount() {
        return m_vecSensorIns.size();
    }

    AX_S8 PipeFromSns(AX_U8 nPipeID);

    AX_VOID SetLoadType(AX_U32 nLoadType) {
        m_nLoadType = nLoadType;
    }

    AX_VOID SetYuvThreadParams(AX_U32 nSnsID, AX_U32 nPipeID, AX_U32 nChannel, AX_BOOL bMultiplex);
    AX_BOOL RestartWithScenario(AX_S32 nScenario);

    AX_VOID RegObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver);
    AX_VOID UnregObserver(AX_U32 nPipeID, AX_U32 nChannel, IObserver* pObserver);

    AX_BOOL ChangeDaynightMode(AX_U32 nSnsID, AX_DAYNIGHT_MODE_E eDaynightMode);

    /*
        @brief: callback function intended to update sensor attributes, which will be called after initialization done and before opening.
    */
    static AX_BOOL UpdateAttrCB(ISensor* pInstance);
    AX_VOID SwitchSnsMode(AX_U32 nSnsID, AX_U32 nSnsMode);
    AX_VOID ChangeSnsFps(AX_U32 nSnsID, AX_F32 fFrameRate);
    AX_VOID ChangeSnsMirrorFlip(AX_U32 nSnsID, AX_BOOL bMirror, AX_BOOL bFlip);

    AX_VOID SetAeSyncRatio(const AX_ISP_IQ_AE_SYNC_RATIO_T& tAeSyncRatio);
    AX_VOID SetAwbSyncRatio(const AX_ISP_IQ_AWB_SYNC_RATIO_T& tAwbSyncRatio);

    AX_S32 Enable3ASyncRatio(AX_BOOL bEnable);

protected:
    AX_VOID RawDispatchThreadFunc(RAW_DISPATCH_THREAD_PARAM_T* pThreadParam);
    AX_VOID YuvGetThreadFunc(YUV_THREAD_PARAM_T* pThreadParam);

private:
    AX_VOID StartDispatchRawThread();
    AX_VOID StopDispatchRawThread();
    AX_VOID StartYuvGetThread();
    AX_VOID StopYuvGetThread();
    AX_VOID StartNtCtrl();
    AX_VOID StopNtCtrl();

    AX_VOID NotifyAll(AX_S32 nPipe, AX_U32 nChannel, AX_VOID* pFrame);

private:
    /*
        @brief: Load sensor configurations with specified types.
    */
    AX_U32 m_nLoadType{0};
    vector<CBaseSensor*> m_vecSensorIns;
    std::map<AX_U32, std::map<AX_U32, std::vector<IObserver*>>> m_mapObservers;
    std::map<AX_U8, RAW_DISPATCH_THREAD_PARAM_T> m_mapDev2ThreadParam;
    std::map<AX_U32, std::map<AX_U32, YUV_THREAD_PARAM_T>> m_mapYuvThreadParams;
    std::map<AX_U32, std::map<AX_U32, AX_BOOL>> m_mapChnLinkable;

    list<CAXFrame*> m_qFrame[MAX_SENSOR_COUNT * MAX_PIPE_PER_DEVICE][AX_VIN_CHN_ID_MAX];
    mutex m_mtxFrame[MAX_SENSOR_COUNT * MAX_PIPE_PER_DEVICE][AX_VIN_CHN_ID_MAX];
    AX_BOOL m_bGetYuvFlag[MAX_SENSOR_COUNT]{AX_TRUE, AX_TRUE};

    AX_ISP_IQ_AE_SYNC_RATIO_T  m_tAeSyncRatio{1 << 20};
    AX_ISP_IQ_AWB_SYNC_RATIO_T m_tAwbSyncRatio{1 << 20, 1 << 20};
};

}  // namespace AX_PANO