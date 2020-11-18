package com.rainman.asf.fragment;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.appcompat.widget.Toolbar;

import android.view.MenuItem;
import android.view.View;

import com.rainman.asf.R;
import com.rainman.asf.activity.MainActivity;

public abstract class BaseFragment extends Fragment implements Toolbar.OnMenuItemClickListener {

    protected MainActivity mMainActivity;

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mMainActivity = (MainActivity) context;
    }

    @CallSuper
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        Toolbar toolbar = view.findViewById(R.id.toolbar);
        onCreateToolbar(toolbar);
        toolbar.setOnMenuItemClickListener(this);

        mMainActivity.setToolbar(toolbar);

        initView(view);
        initData(savedInstanceState);
    }

    protected void onCreateToolbar(Toolbar toolbar) {

    }

    @Override
    public boolean onMenuItemClick(MenuItem menuItem) {
        return false;
    }

    protected void initView(View view) {

    }

    protected void initData(Bundle savedInstanceState) {

    }
}
