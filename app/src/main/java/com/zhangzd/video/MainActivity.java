package com.zhangzd.video;

import android.net.Uri;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.TextView;

import com.yanzhenjie.permission.Action;
import com.yanzhenjie.permission.AndPermission;
import com.yanzhenjie.permission.runtime.Permission;

import java.util.List;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
        System.loadLibrary("avcodec");
        System.loadLibrary("avfilter");
        System.loadLibrary("avformat");
        System.loadLibrary("avutil");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
//        sysloginit();
//        final String src = "/storage/emulated/0/DCIM/Camera/e7a9c442d1cda4462fc03459d71d8b7c.mp4";
        final String src = "https://ips.ifeng.com/video19.ifeng.com/video09/2018/12/07/p5749945-102-009-184237.mp4";

        final TextView textView = findViewById(R.id.tv2);
        textView.setMovementMethod(ScrollingMovementMethod.getInstance());
        final TextView textView1 = findViewById(R.id.tv1);
        textView1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                AndPermission.with(MainActivity.this)
                        .runtime()
                        .permission(Permission.WRITE_EXTERNAL_STORAGE,
                                Permission.READ_EXTERNAL_STORAGE)
                        .onGranted(new Action<List<String>>() {
                            @Override
                            public void onAction(List<String> data) {
                                avioreading(src);

                            }
                        })
                        .onDenied(new Action<List<String>>() {
                            @Override
                            public void onAction(List<String> data) {

                            }
                        }).start();
            }
        });

    }
    public native String avcodecinfo();
    public native String avioreading(String src);
    public native String sysloginit();

}
