package com.rainman.asf.view;

import android.content.Context;
import android.os.Bundle;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.*;
import com.rainman.asf.R;

public class DrawerMenuItem extends RelativeLayout implements CompoundButton.OnCheckedChangeListener {

    private Switch swSwitch;
    private int mMenuType;
    private CompoundButton.OnCheckedChangeListener mCheckedChangeListener;

    public DrawerMenuItem(Context context) {
        this(context, null);
    }

    public DrawerMenuItem(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DrawerMenuItem(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        Bundle bundle = new Bundle();
        Parcelable superData = super.onSaveInstanceState();
        bundle.putParcelable("super_data", superData);
        bundle.putBoolean("save_data", swSwitch.isChecked());
        return bundle;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        Bundle bundle = (Bundle) state;
        Parcelable superData = bundle.getParcelable("super_data");
        final boolean saveData = bundle.getBoolean("save_data");
        post(new Runnable() {
            @Override
            public void run() {
                swSwitch.setChecked(saveData);
            }
        });
        super.onRestoreInstanceState(superData);
    }

    void init(Context context, AttributeSet attrs) {
        View.inflate(context, R.layout.drawer_menu_item, this);
        ImageView ivIcon = findViewById(R.id.iv_icon);
        TextView tvTitle = findViewById(R.id.tv_title);
        swSwitch = findViewById(R.id.sw_switch);

        if (attrs != null) {
            for (int i = 0; i < attrs.getAttributeCount(); i++) {
                switch (attrs.getAttributeName(i)) {
                    case "title":
                        tvTitle.setText(attrs.getAttributeResourceValue(i, R.string.item_name));
                        break;
                    case "icon":
                        ivIcon.setImageResource(attrs.getAttributeResourceValue(i, R.drawable.ic_image_black_24dp));
                        break;
                    case "menu_type":
                        mMenuType = attrs.getAttributeIntValue(i, 0);
                        if (mMenuType == 1) {
                            swSwitch.setVisibility(VISIBLE);
                        } else if (mMenuType == 0) {
                            setClickable(true);
                            setFocusable(true);
                        }
                        break;
                }
            }
        }
    }

    public void setOnCheckedChangeListener(CompoundButton.OnCheckedChangeListener listener) {
        mCheckedChangeListener = listener;
        swSwitch.setOnCheckedChangeListener(this);
    }

    public boolean isChecked() {
        return swSwitch.isChecked();
    }

    public void setChecked(boolean checked) {
        swSwitch.setChecked(checked);
    }

    public boolean isEnabled() {
        if (mMenuType == 1) {
            return swSwitch.isEnabled();
        } else {
            return super.isEnabled();
        }
    }

    public void setEnabled(boolean enabled) {
        if (mMenuType == 1) {
            swSwitch.setEnabled(enabled);
        } else {
            super.setEnabled(enabled);
        }
    }

    public void setClickable(boolean clickable) {
        if (mMenuType == 1) {
            swSwitch.setClickable(clickable);
        } else {
            super.setClickable(clickable);
        }
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (mCheckedChangeListener != null) {
            if (!buttonView.isPressed())
                return;
            mCheckedChangeListener.onCheckedChanged(buttonView, isChecked);
        }
    }
}
