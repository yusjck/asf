package com.rainman.asf.userconfig;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import com.rainman.asf.R;

public class EditViewHolder extends BaseViewHolder implements View.OnClickListener {

    private final TextView tvText;
    private final TextView tvValue;
    private EditItem mItemData;

    public EditViewHolder(View itemView) {
        super(itemView);
        tvText = itemView.findViewById(R.id.tv_text);
        tvValue = itemView.findViewById(R.id.tv_value);
        tvText.setOnClickListener(this);
        tvValue.setOnClickListener(this);
    }

    @Override
    public void bindData(ViewData data) {
        mItemData = (EditItem) data.getExt();
        tvText.setText(mItemData.getText());
        String value = mUserVar.get(mItemData.getName());
        if (mItemData.isMask()) {
            value = value.replaceAll(".", "*");
        }
        tvValue.setText(value);
    }

    @Override
    public void onClick(View v) {
        View view = View.inflate(v.getContext(), R.layout.dialog_input, null);
        TextView tvDescription = view.findViewById(R.id.tv_description);
        if (mItemData.getDescription().equals("")) {
            tvDescription.setText(R.string.please_entry);
        } else {
            tvDescription.setText(mItemData.getDescription());
        }

        final EditText editText = view.findViewById(R.id.et_value);
        editText.setText(mUserVar.get(mItemData.getName()));
        editText.setSelectAllOnFocus(true);

        AlertDialog.Builder builder = new AlertDialog.Builder(v.getContext());
        builder.setTitle(mItemData.getText());
        builder.setView(view);
        builder.setPositiveButton(R.string.dlg_confirm, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                tvValue.setText(editText.getText());
                mUserVar.put(mItemData.getName(), editText.getText().toString());
                mUserVar.save();
                dialog.dismiss();
            }
        });
        builder.setNegativeButton(R.string.dlg_cancel, null);
        builder.show();
    }
}
