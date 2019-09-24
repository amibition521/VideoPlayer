package com.zhangzd.video;

import android.app.Application;

import com.tencent.bugly.crashreport.CrashReport;

public class VideoApplication extends Application {


    @Override
    public void onCreate() {
        super.onCreate();
        CrashReport.initCrashReport(this, "17a02bbef8", true);
    }
}
