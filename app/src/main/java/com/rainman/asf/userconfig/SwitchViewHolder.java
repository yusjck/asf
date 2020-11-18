package com.rainman.asf.userconfig;

import android.view.View;
import android.widget.CompoundButton;
import android.widget.Switch;
import android.widget.TextView;
import com.rainman.asf.R;

public class SwitchViewHolder extends BaseViewHolder implements CompoundButton.OnCheckedChangeListener {

    private final TextView tvText;
    private final TextView tvDescription;
    private final Switch swValue;
    private SwitchItem mItemData;

    public SwitchViewHolder(View itemView) {
        super(itemView);
        tvText = itemView.findViewById(R.id.tv_text);
        tvDescription = itemView.findViewById(R.id.tv_description);
        swValue = itemView.findViewById(R.id.sw_value);
        swValue.setOnCheckedChangeListener(this);
    }

    @Override
    public void bindData(ViewData data) {
        mItemData = (SwitchItem) data.getExt();
        tvText.setText(mItemData.getText());
        tvDescription.setText(mItemData.getDescription());
        swValue.setChecked(!mUserVar.get(mItemData.getName()).equals("0"));
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
        if (isChecked) {
            mUserVar.put(mItemData.getName(), "1");
        } else {
            mUserVar.put(mItemData.getName(), "0");
        }
        mUserVar.save();
    }
}
