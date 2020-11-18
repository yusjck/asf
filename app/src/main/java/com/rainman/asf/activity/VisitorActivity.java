package com.rainman.asf.activity;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;

import androidx.recyclerview.widget.DividerItemDecoration;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.*;

import com.rainman.asf.R;
import com.rainman.asf.core.VisitorManager;
import com.rainman.asf.core.database.Visitor;

import java.util.List;

public class VisitorActivity extends AppCompatActivity {

    private VisitorManager mVisitorManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_visitor);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        RecyclerView rvDownloadList = findViewById(R.id.rv_visitor_list);
        rvDownloadList.setLayoutManager(new LinearLayoutManager(this));
        rvDownloadList.addItemDecoration(new DividerItemDecoration(this, DividerItemDecoration.VERTICAL));

        mVisitorManager = VisitorManager.getInstance();
        VisitorAdapter visitorAdapter = new VisitorAdapter();
        rvDownloadList.setAdapter(visitorAdapter);
        visitorAdapter.reloadData();
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getItemId() == android.R.id.home) {
            this.finish();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private class VisitorAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

        private List<Visitor> mVisitors;

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.view_visitor_item, parent, false);
            return new ViewHolder(view);
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            ((ViewHolder) holder).bindData(mVisitors.get(position));
        }

        @Override
        public int getItemCount() {
            return mVisitors == null ? 0 : mVisitors.size();
        }

        void reloadData() {
            mVisitors = mVisitorManager.getVisitors();
            notifyDataSetChanged();
        }

        private class ViewHolder extends RecyclerView.ViewHolder {

            private final TextView tv_name;
            private final TextView tv_signature;
            private final Spinner spinner;
            private Visitor mVisitor;
            private int mAccessPermission;

            ViewHolder(View view) {
                super(view);
                tv_name = view.findViewById(R.id.tv_name);
                tv_signature = view.findViewById(R.id.tv_signature);

                String[] options = getResources().getStringArray(R.array.visitor_permission);
                ArrayAdapter<String> adapter = new ArrayAdapter<>(VisitorActivity.this, android.R.layout.simple_spinner_item, options);
                adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);

                spinner = view.findViewById(R.id.spinner);
                spinner.setAdapter(adapter);
                spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                        switch (position) {
                            case 0:
                                mVisitor.setAccessPermission(Visitor.PERMISSION_GRANTED);
                                mVisitor.setExpireTime(0);
                                break;
                            case 1:
                                mVisitor.setAccessPermission(Visitor.PERMISSION_DENIED);
                                break;
                            case 2:
                                mVisitor.setAccessPermission(Visitor.PERMISSION_PROMPT);
                                break;
                            case 3:
                                mVisitorManager.deleteVisitor(mVisitor);
                                reloadData();
                                break;
                        }
                        if (mAccessPermission != mVisitor.getAccessPermission()) {
                            mVisitorManager.addOrUpdateVisitor(mVisitor);
                        }
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {

                    }
                });
            }

            void bindData(Visitor visitor) {
                mVisitor = visitor;
                tv_name.setText(visitor.getName());
                tv_signature.setText(visitor.getSignature());

                if (mVisitor.isPermissionExpired() || mVisitor.getAccessPermission() == Visitor.PERMISSION_PROMPT) {
                    mAccessPermission = Visitor.PERMISSION_PROMPT;
                    spinner.setSelection(2);
                } else if (mVisitor.getAccessPermission() == Visitor.PERMISSION_GRANTED) {
                    mAccessPermission = Visitor.PERMISSION_GRANTED;
                    spinner.setSelection(0);
                } else if (mVisitor.getAccessPermission() == Visitor.PERMISSION_DENIED) {
                    mAccessPermission = Visitor.PERMISSION_DENIED;
                    spinner.setSelection(1);
                }
            }
        }
    }
}
