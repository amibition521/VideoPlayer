package com.zhangzd.video;

import android.net.Uri;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.view.View;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        sysloginit();
        final String src = "/storage/emulated/0/DCIM/Camera/e7a9c442d1cda4462fc03459d71d8b7c.mp4";

        final TextView textView = findViewById(R.id.tv2);
        textView.setMovementMethod(ScrollingMovementMethod.getInstance());
        final TextView textView1 = findViewById(R.id.tv1);
        textView1.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                textView.setText(avcodecinfo());
                avioreading(src);
            }
        });
    }
    public native String avcodecinfo();
    public native String avioreading(String src);
    public native String sysloginit();

}
