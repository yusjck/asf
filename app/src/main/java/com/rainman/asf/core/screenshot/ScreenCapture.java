package com.rainman.asf.core.screenshot;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

import java.nio.ByteBuffer;

public class ScreenCapture implements ImageReader.OnImageAvailableListener {

    private static final String TAG = "ScreenCapture";
    private static final ScreenCapture mInstance = new ScreenCapture();
    private ImageReader mImageReader;
    private VirtualDisplay mVirtualDisplay;
    private Looper mImageAcquireLooper;
    private Bitmap mLastImage;
    private final Object mLastImageLock = new Object();
    private Integer mReferenceCount = 0;

    public interface CaptureHandle {

        byte[] getScreenPixels(int x, int y, int width, int height);

        void release();
    }

    public static CaptureHandle getCaptureHandle() {
        synchronized (mInstance) {
            if (mInstance.mReferenceCount == 0 && !mInstance.startCapture())
                return null;
            mInstance.mReferenceCount++;
        }

        return new CaptureHandle() {
            @Override
            public byte[] getScreenPixels(int x, int y, int width, int height) {
                return mInstance.getScreenPixels(x, y, width, height);
            }

            @Override
            public void release() {
                synchronized (mInstance) {
                    if (--mInstance.mReferenceCount > 0)
                        return;
                    mInstance.stopCapture();
                }
            }
        };
    }

    @Override
    public void onImageAvailable(ImageReader reader) {
        Image image = mImageReader.acquireLatestImage();
        if (image == null) {
            Log.e(TAG, "image is null");
            return;
        }

        Image.Plane[] planes = image.getPlanes();
        int imageHeight = image.getHeight();
        ByteBuffer buffer = planes[0].getBuffer();
        int pixelStride = planes[0].getPixelStride();
        int rowStride = planes[0].getRowStride();

        synchronized (mLastImageLock) {
            if (mLastImage != null)
                mLastImage.recycle();
            mLastImage = Bitmap.createBitmap(rowStride / pixelStride, imageHeight, Bitmap.Config.ARGB_8888);
            mLastImage.copyPixelsFromBuffer(buffer);
        }

        image.close();
    }

    @SuppressLint("WrongConstant")
    private boolean startCapture() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            return false;
        }

        ScreenCaptureService service = ScreenCaptureService.getInstance();
        if (service == null) {
            return false;
        }

        MediaProjection mediaProjection = service.getMediaProjection();
        if (mediaProjection == null) {
            return false;
        }

        WindowManager windowManager = (WindowManager) service.getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();
        DisplayMetrics displayMetrics = new DisplayMetrics();
        display.getRealMetrics(displayMetrics);

        mImageReader = ImageReader.newInstance(displayMetrics.widthPixels, displayMetrics.heightPixels, PixelFormat.RGBA_8888, 2);

        mVirtualDisplay = mediaProjection.createVirtualDisplay("screen-mirror",
                displayMetrics.widthPixels,
                displayMetrics.heightPixels,
                displayMetrics.densityDpi,
                DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                mImageReader.getSurface(), null, null);
        if (mVirtualDisplay == null) {
            Log.e(TAG, "mVirtualDisplay is null");
            return false;
        }

        new Thread() {
            @Override
            public void run() {
                super.run();
                Looper.prepare();
                mImageAcquireLooper = Looper.myLooper();
                mImageReader.setOnImageAvailableListener(ScreenCapture.this, new Handler());
                Looper.loop();
            }
        }.start();

        return true;
    }

    private void stopCapture() {
        if (mImageAcquireLooper != null) {
            mImageAcquireLooper.quit();
            try {
                mImageAcquireLooper.getThread().join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            mImageAcquireLooper = null;
        }

        if (mLastImage != null) {
            mLastImage.recycle();
            mLastImage = null;
        }

        if (mVirtualDisplay != null) {
            mVirtualDisplay.release();
            mVirtualDisplay = null;
        }

        if (mImageReader != null) {
            mImageReader.close();
            mImageReader = null;
        }
    }

    private byte[] getScreenPixels(int x, int y, int width, int height) {
        Bitmap bitmap;
        synchronized (mLastImageLock) {
            if (mLastImage == null) {
                Log.e(TAG, "mLastImage is null");
                return null;
            }

            bitmap = Bitmap.createBitmap(mLastImage, x, y, width, height);
        }

        ByteBuffer buf = ByteBuffer.allocate(bitmap.getWidth() * bitmap.getHeight() * 4);
        bitmap.copyPixelsToBuffer(buf);
        bitmap.recycle();
        return buf.array();
    }
}
