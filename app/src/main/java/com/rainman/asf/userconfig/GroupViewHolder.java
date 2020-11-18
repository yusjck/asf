package com.rainman.asf.userconfig;

import android.view.View;
import android.widget.TextView;
import com.rainman.asf.R;

public class GroupViewHolder extends BaseViewHolder {

    private final TextView tvText;

    public GroupViewHolder(View itemView) {
        super(itemView);
        tvText = itemView.findViewById(R.id.tv_text);
    }

    @Override
    public void bindData(ViewData data) {
        tvText.setText(data.getGroupName());
    }
}
