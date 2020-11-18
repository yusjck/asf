package com.rainman.asf.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;
import com.rainman.asf.R;

public class DrawerMenuGroup extends RelativeLayout {

    public DrawerMenuGroup(Context context) {
        this(context, null);
    }

    public DrawerMenuGroup(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DrawerMenuGroup(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    void init(Context context, AttributeSet attrs) {
        View.inflate(context, R.layout.drawer_menu_group, this);
        TextView tvTitle = findViewById(R.id.tv_title);

        if (attrs != null) {
            for (int i = 0; i < attrs.getAttributeCount(); i++) {
                if ("title".equals(attrs.getAttributeName(i))) {
                    tvTitle.setText(attrs.getAttributeResourceValue(i, R.string.group_name));
                }
            }
        }
    }
}
