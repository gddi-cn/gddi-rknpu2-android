package com.example.gddandroid;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import com.example.gddandroid.databinding.ActivityMainBinding;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {


    static {
        System.loadLibrary("gddandroid");
    }


    private static Long nativeObj = 0L;
    String[] permissions = new String[]{
            android.Manifest.permission.CAMERA,
            android.Manifest.permission.READ_EXTERNAL_STORAGE,
            android.Manifest.permission.WRITE_EXTERNAL_STORAGE
    };
    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        stringFromJNI();
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI());
        checkPermissions();
        initButton();
    }

    private void initButton() {
        findViewById(R.id.btnInit).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 初始化
                File modelFile = null;
                File licenseFile = null;
                File picFile = null;
                try {
                    InputStream is = getResources().openRawResource(R.raw.model);
                    File cascadeDir = getDir("cascade", Context.MODE_PRIVATE);
                    modelFile = new File(cascadeDir, "model.gdd");
                    FileOutputStream os = new FileOutputStream(modelFile);
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    while ((bytesRead = is.read(buffer)) != -1) {
                        os.write(buffer, 0, bytesRead);
                    }
                    is.close();
                    os.close();
                } catch (Exception ex) {
                }

                try {
                    InputStream is = getResources().openRawResource(R.raw.license);
                    File cascadeDir = getDir("cascade", Context.MODE_PRIVATE);
                    licenseFile = new File(cascadeDir, "license");
                    FileOutputStream os = new FileOutputStream(licenseFile);
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    while ((bytesRead = is.read(buffer)) != -1) {
                        os.write(buffer, 0, bytesRead);
                    }
                    is.close();
                    os.close();
                } catch (Exception ex) {
                }

                try {
                    InputStream is = getResources().openRawResource(R.raw.image_001);
                    File cascadeDir = getDir("cascade", Context.MODE_PRIVATE);
                    picFile = new File(cascadeDir, "image_001.jpg");
                    FileOutputStream os = new FileOutputStream(picFile);
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    while ((bytesRead = is.read(buffer)) != -1) {
                        os.write(buffer, 0, bytesRead);
                    }
                    is.close();
                    os.close();
                } catch (Exception ex) {
                }

                nativeObj = GddInit("", modelFile.getAbsolutePath(), licenseFile.getAbsolutePath(), picFile.getAbsolutePath());
                if (nativeObj != 0) {
                    Toast.makeText(MainActivity.this, "初始化成功", Toast.LENGTH_SHORT).show();
                }
            }
        });

        findViewById(R.id.btnInfer).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {

            }
        });

        findViewById(R.id.btnDeInit).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (nativeObj != 0) {
                    if (GddDeInit(nativeObj) == 0) {
                        nativeObj = 0L;
                        Toast.makeText(MainActivity.this, "资源释放成功", Toast.LENGTH_SHORT).show();
                    }
                }
            }
        });
    }

    private void startRequestPermission() {
        ActivityCompat.requestPermissions(this, permissions, 321);
    }

    private void checkPermissions() {
        //如果系统大于android6.0，进行动态权限申请
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            int i = ContextCompat.checkSelfPermission(this, permissions[0]);
            int l = ContextCompat.checkSelfPermission(this, permissions[1]);
            int m = ContextCompat.checkSelfPermission(this, permissions[2]);
            if (i != PackageManager.PERMISSION_GRANTED ||
                    l != PackageManager.PERMISSION_GRANTED ||
                    m != PackageManager.PERMISSION_GRANTED) {
                // 如果有权限没有授予，就去提示用户请求
                startRequestPermission();
            }
        }
    }

    public native String stringFromJNI();

    public native long GddInit(String configFile, String modelPath, String licenseFile, String picPath);

    public native int GddDeInit(long handle);
}