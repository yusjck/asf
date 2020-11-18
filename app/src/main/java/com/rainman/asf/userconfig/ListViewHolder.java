package com.rainman.asf.userconfig;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.widget.TextView;
import com.rainman.asf.R;

public class ListViewHolder extends BaseViewHolder implements View.OnClickListener {

    private final TextView tvText;
    private final TextView tvValue;
    private ListItem mItemData;

    public ListViewHolder(View itemView) {
        super(itemView);
        tvText = itemView.findViewById(R.id.tv_text);
        tvValue = itemView.findViewById(R.id.tv_value);
        tvText.setOnClickListener(this);
        tvValue.setOnClickListener(this);
    }

    @Override
    public void bindData(ViewData data) {
        mItemData = (ListItem) data.getExt();
        tvText.setText(mItemData.getText());
        String optValue = mUserVar.get(mItemData.getName());
        tvValue.setText(mItemData.toOptionText(optValue));
    }

    @Override
    public void onClick(View v) {
        AlertDialog.Builder builder = new AlertDialog.Builder(v.getContext());
        builder.setTitle(mItemData.getText());

        final String[] optTexts = new String[mItemData.getOptions().size()];
        mItemData.getOptions().values().toArray(optTexts);
        int currentSelected = mItemData.toOptionIndex(mUserVar.get(mItemData.getName()));

        builder.setSingleChoiceItems(optTexts, currentSelected, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                String optText = optTexts[which];
                ListViewHolder.this.tvValue.setText(optText);
                mUserVar.put(mItemData.getName(), mItemData.toOptionValue(optText));
                mUserVar.save();
                dialog.dismiss();
            }
        });
        builder.show();
    }
}
