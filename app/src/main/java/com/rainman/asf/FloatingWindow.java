package com.rainman.asf;

import android.animation.ValueAnimator;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.os.Build;
import android.os.CountDownTimer;
import android.provider.Settings;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.WindowManager;
import android.view.animation.AlphaAnimation;
import android.view.animation.BounceInterpolator;
import android.widget.ImageView;

import com.rainman.asf.activity.OptionActivity;
import com.rainman.asf.core.ScriptEngine;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.util.DisplayUtil;
import com.rainman.asf.util.ToastUtil;

public class FloatingWindow {

    @SuppressLint("StaticFieldLeak")
    private static FloatingWindow mInstance;
    private WindowManager mWindowManager;
    private View mView;
    private WindowManager.LayoutParams mLayoutParams;
    private int mScreenWidth;
    private FloatingMenu mFloatingMenu = new FloatingMenu();

    public static FloatingWindow getInstance() {
        if (mInstance == null) {
            mInstance = new FloatingWindow();
        }
        return mInstance;
    }

    private void showWindow(Context context) {
        if (mWindowManager == null && mView == null) {
            mWindowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
            mView = View.inflate(context, R.layout.view_floating_window, null);

            mLayoutParams = new WindowManager.LayoutParams();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                mLayoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
            } else {
                mLayoutParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
            }

            mLayoutParams.format = PixelFormat.RGBA_8888;
            mLayoutParams.gravity = Gravity.START | Gravity.TOP;
            mLayoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
            mWindowManager.addView(mView, mLayoutParams);

            initListener(context);

            mView.post(new Runnable() {
                @Override
                public void run() {
                    Point floatBallPos = AppSetting.getFloatingWndPos();
                    ImageView ivIcon = mView.findViewById(R.id.iv_icon);
                    mLayoutParams.x = floatBallPos.x;
                    mLayoutParams.y = floatBallPos.y;
                    mLayoutParams.width = ivIcon.getMeasuredWidth();
                    mLayoutParams.height = ivIcon.getMeasuredHeight();
                    mWindowManager.updateViewLayout(mView, mLayoutParams);

                    AlphaAnimation alphaAnimation = new AlphaAnimation(0, 1);
                    alphaAnimation.setDuration(1000);
                    ivIcon.setAnimation(alphaAnimation);
                }
            });
        }
    }

    private void dismissWindow() {
        if (mWindowManager != null && mView != null) {
            mFloatingMenu.closeMenu();

            mWindowManager.removeViewImmediate(mView);
            mWindowManager = null;
            mView = null;
            mLayoutParams = null;
        }
    }

    public void switchFloatingWindow(Context context) {
        if (AppSetting.isFloatingWndEnabled()) {
            context = context.getApplicationContext();
            if (Build.VERSION.SDK_INT < 23 || Settings.canDrawOverlays(context)) {
                showWindow(context);
            } else {
                ToastUtil.show(context, R.string.floating_window_permission_denied);
            }
        } else {
            dismissWindow();
        }
    }

    private void initListener(Context context) {
        final int touchSlop = ViewConfiguration.get(context).getScaledTouchSlop();
        final int statusBarHeight = DisplayUtil.getStatusBarHeight(context);
        Point point = new Point();
        mWindowManager.getDefaultDisplay().getSize(point);
        mScreenWidth = point.x;

        mView.setOnTouchListener(new View.OnTouchListener() {
            int startX, startY;
            boolean isPerformClick;
            int finalMoveX;

            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        startX = (int) event.getX();
                        startY = (int) event.getY();
                        isPerformClick = true;
                        return true;
                    case MotionEvent.ACTION_MOVE:
                        if (Math.abs(startX - event.getX()) >= touchSlop || Math.abs(startY - event.getY()) >= touchSlop) {
                            isPerformClick = false;
                        }
                        mLayoutParams.x = (int) (event.getRawX() - startX);
                        mLayoutParams.y = (int) (event.getRawY() - startY - statusBarHeight);
                        mWindowManager.updateViewLayout(mView, mLayoutParams);
                        return true;
                    case MotionEvent.ACTION_UP:
                        if (isPerformClick) {
                            mView.performClick();
                        }
                        if (mLayoutParams.x + mView.getMeasuredWidth() / 2 >= mScreenWidth / 2) {
                            finalMoveX = mScreenWidth - (int) (mView.getMeasuredWidth() * 0.6);
                        } else {
                            finalMoveX = -(int) (mView.getMeasuredWidth() * 0.4);
                        }
                        AppSetting.setFloatingWndPos(new Point(finalMoveX, mLayoutParams.y));
                        stickToSide();
                        return !isPerformClick;
                }
                return false;
            }

            private void stickToSide() {
                ValueAnimator animator = ValueAnimator.ofInt(mLayoutParams.x, finalMoveX).setDuration(500);
                animator.setInterpolator(new BounceInterpolator());
                animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                    @Override
                    public void onAnimationUpdate(ValueAnimator animation) {
                        mLayoutParams.x = (int) animation.getAnimatedValue();
                        mWindowManager.updateViewLayout(mView, mLayoutParams);
                    }
                });
                animator.start();
            }
        });

        mView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mFloatingMenu.openMenu(v.getContext());
            }
        });
    }

    private class FloatingMenu {

        private View mMenuView;
        private WindowManager.LayoutParams mMenuLayoutParams;
        private CountDownTimer mCountDownTimer;

        void openMenu(final Context context) {
            if (mMenuView == null) {
                mMenuView = View.inflate(context, R.layout.view_floating_menu, null);

                mMenuLayoutParams = new WindowManager.LayoutParams();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    mMenuLayoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
                } else {
                    mMenuLayoutParams.type = WindowManager.LayoutParams.TYPE_SYSTEM_ALERT;
                }

                mMenuLayoutParams.format = PixelFormat.RGBA_8888;
                mMenuLayoutParams.gravity = Gravity.START | Gravity.TOP;
                mMenuLayoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
                mMenuLayoutParams.x = mScreenWidth;
                mMenuLayoutParams.y = mLayoutParams.y;
                mWindowManager.addView(mMenuView, mMenuLayoutParams);

                mMenuView.post(new Runnable() {
                    @Override
                    public void run() {
                        runMenuAnimator();
                    }
                });

                View ivStart = mMenuView.findViewById(R.id.iv_start);
                ivStart.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        ScriptEngine.getInstance().startScript();
                        closeMenu();
                    }
                });

                View ivStop = mMenuView.findViewById(R.id.iv_stop);
                ivStop.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        ScriptEngine.getInstance().stopScript();
                        closeMenu();
                    }
                });

                View ivSetting = mMenuView.findViewById(R.id.iv_setting);
                ivSetting.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        Script script = ScriptManager.getInstance().getCurrentScript();
                        if (script == null) {
                            ToastUtil.show(context, R.string.no_script_selected);
                        } else {
                            Intent intent = new Intent(context, OptionActivity.class);
                            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                            intent.putExtra("script_id", script.getId());
                            context.startActivity(intent);
                        }
                        closeMenu();
                    }
                });

                View ivClose = mMenuView.findViewById(R.id.iv_close);
                ivClose.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        closeMenu();
                    }
                });

                mCountDownTimer = new CountDownTimer(3000, 1000) {
                    @Override
                    public void onTick(long millisUntilFinished) {

                    }

                    @Override
                    public void onFinish() {
                        closeMenu();
                    }
                };
                mCountDownTimer.start();

                mView.setVisibility(View.GONE);
            }
        }

        private void runMenuAnimator() {
            View view = mMenuView.findViewById(R.id.ll_menu);
            mMenuLayoutParams.width = view.getMeasuredWidth();
            mMenuLayoutParams.height = view.getMeasuredHeight();

            // 决定菜单应该显示在屏幕的左侧还是右侧
            int startX = -view.getMeasuredWidth(), endX = 0;
            if (mLayoutParams.x > 0) {
                view.setBackgroundResource(R.drawable.bg_floating_menu_left);
                view.setLayoutDirection(View.LAYOUT_DIRECTION_LTR);
                startX = mScreenWidth;
                endX = mScreenWidth - view.getMeasuredWidth();
            }

            // 执行菜单从屏幕外则移入屏幕动画
            ValueAnimator animator = ValueAnimator.ofInt(startX, endX).setDuration(300);
            animator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener() {
                @Override
                public void onAnimationUpdate(ValueAnimator animation) {
                    mMenuLayoutParams.x = (int) animation.getAnimatedValue();
                    mWindowManager.updateViewLayout(mMenuView, mMenuLayoutParams);
                }
            });
            animator.start();
        }

        void closeMenu() {
            if (mMenuView != null) {
                mCountDownTimer.cancel();

                mWindowManager.removeViewImmediate(mMenuView);
                mMenuView = null;
                mMenuLayoutParams = null;

                mView.setVisibility(View.VISIBLE);
            }
        }
    }

    public void rotateScreen() {
        if (mWindowManager != null && mView != null) {
            mFloatingMenu.closeMenu();

            Point point = new Point();
            mWindowManager.getDefaultDisplay().getSize(point);
            if (mLayoutParams.x > 0) {
                mLayoutParams.x = point.x - (int) (mView.getMeasuredWidth() * 0.6);
            }
            mLayoutParams.y *= (float) mScreenWidth / point.x;
            mScreenWidth = point.x;
            mWindowManager.updateViewLayout(mView, mLayoutParams);
        }
    }
}
