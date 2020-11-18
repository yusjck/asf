package com.rainman.asf.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.rainman.asf.R;

public class IconButton extends LinearLayout {

    public IconButton(Context context) {
        this(context, null);
    }

    public IconButton(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public IconButton(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    void init(Context context, AttributeSet attrs) {
        View.inflate(context, R.layout.view_icon_button, this);
        ImageView ivIcon = findViewById(R.id.iv_icon);
        TextView tvText = findViewById(R.id.tv_text);

        if (attrs != null) {
            for (int i = 0; i < attrs.getAttributeCount(); i++) {
                switch (attrs.getAttributeName(i)) {
                    case "text":
                        tvText.setText(attrs.getAttributeResourceValue(i, R.string.button_name));
                        break;
                    case "icon":
                        ivIcon.setImageResource(attrs.getAttributeResourceValue(i, R.drawable.ic_image_black_24dp));
                        break;
                }
            }
        }
    }
}
