package com.rainman.asf.view;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.rainman.asf.R;

public class AboutItem extends RelativeLayout {

    private TextView tvContent;

    public AboutItem(Context context) {
        this(context, null);
    }

    public AboutItem(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public AboutItem(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    private void init(Context context, AttributeSet attrs) {
        View.inflate(context, R.layout.view_about_item, this);
        TextView tvTitle = findViewById(R.id.tv_title);
        tvContent = findViewById(R.id.tv_content);

        if (attrs != null) {
            for (int i = 0; i < attrs.getAttributeCount(); i++) {
                switch (attrs.getAttributeName(i)) {
                    case "title":
                        tvTitle.setText(attrs.getAttributeResourceValue(i, R.string.button_name));
                        break;
                    case "content":
                        tvContent.setText(attrs.getAttributeResourceValue(i, R.string.button_name));
                        break;
                }
            }
        }
    }

    public void setContent(String content) {
        tvContent.setText(content);
    }

    public void setContent(int resid) {
        tvContent.setText(resid);
    }
}
