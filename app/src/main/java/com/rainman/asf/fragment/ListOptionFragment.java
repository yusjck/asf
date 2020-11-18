package com.rainman.asf.fragment;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;

import com.rainman.asf.R;
import com.rainman.asf.userconfig.*;

import java.io.File;
import java.util.ArrayList;

public class ListOptionFragment extends Fragment {

    private static final String TAG = "ListOptionFragment";
    private UserVar mUserVar;
    private RecyclerView rvScriptOption;
    private LinearLayout llNoOptionPrompt;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_list_option, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        initView(view);
        initData(savedInstanceState);
    }

    private void initView(View view) {
        rvScriptOption = view.findViewById(R.id.rv_option_list);
        rvScriptOption.setHasFixedSize(true);
        rvScriptOption.setLayoutManager(new LinearLayoutManager(view.getContext()));
        llNoOptionPrompt = view.findViewById(R.id.ll_no_option_prompt);
    }

    private void initData(Bundle savedInstanceState) {
        Bundle bundle = getArguments();
        assert bundle != null;
        String scriptPath = bundle.getString("script_path");
        String configName = bundle.getString("config_name");

        ConfigManager configManager = new ConfigManager();
        if (!configManager.loadConfigs(new File(scriptPath))) {
            Log.i(TAG, "load UserVarDef.xml failed");
            rvScriptOption.setVisibility(View.GONE);
            llNoOptionPrompt.setVisibility(View.VISIBLE);
        }

        mUserVar = new UserVar(configManager, configName);

        OptionAdapter adapter = new OptionAdapter();
        ArrayList<ConfigGroup> groups = configManager.getGroups();
        adapter.setData(groups);
        rvScriptOption.setAdapter(adapter);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
    }

    public class OptionAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

        private ArrayList<ViewData> viewItems = new ArrayList<>();

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            LayoutInflater inflater = LayoutInflater.from(parent.getContext());
            switch (viewType) {
                case 1:
                    return new EditViewHolder(inflater.inflate(R.layout.item_edit, parent, false));
                case 2:
                    return new ListViewHolder(inflater.inflate(R.layout.item_list, parent, false));
                case 3:
                    return new SwitchViewHolder(inflater.inflate(R.layout.item_switch, parent, false));
            }
            return new GroupViewHolder(inflater.inflate(R.layout.item_group, parent, false));
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
            BaseViewHolder baseViewHolder = (BaseViewHolder) holder;
            baseViewHolder.setUserVar(mUserVar);
            baseViewHolder.bindData(viewItems.get(position));
        }

        public void setData(ArrayList<ConfigGroup> groups) {
            for (ConfigGroup group : groups) {
                ViewData groupData = new ViewData();
                groupData.setType(0);
                groupData.setGroupName(group.getText());
                viewItems.add(groupData);

                for (ConfigItem item : group.getItems()) {
                    ViewData itemData = new ViewData();
                    switch (item.getItemType()) {
                        case "Edit":
                            itemData.setType(1);
                            break;
                        case "List":
                            itemData.setType(2);
                            break;
                        case "Switch":
                            itemData.setType(3);
                            break;
                    }
                    itemData.setGroupName(group.getText());
                    itemData.setExt(item);
                    viewItems.add(itemData);
                }
            }
        }

        @Override
        public int getItemViewType(int position) {
            return viewItems.get(position).getType();
        }

        @Override
        public int getItemCount() {
            return viewItems.size();
        }
    }
}
