package com.rainman.asf.userconfig;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import android.view.View;

public abstract class BaseViewHolder extends RecyclerView.ViewHolder {

    protected UserVar mUserVar;

    protected BaseViewHolder(@NonNull View itemView) {
        super(itemView);
    }

    public void setUserVar(UserVar userVar) {
        mUserVar = userVar;
    }

    public abstract void bindData(ViewData data);
}
