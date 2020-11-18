package com.rainman.asf.activity;

import androidx.appcompat.app.ActionBar;
import androidx.fragment.app.Fragment;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;

import com.rainman.asf.R;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Script;
import com.rainman.asf.fragment.HtmlOptionFragment;
import com.rainman.asf.fragment.ListOptionFragment;
import com.rainman.asf.util.ToastUtil;

public class OptionActivity extends AppCompatActivity {

    private Script mScript;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_option);

        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setHomeButtonEnabled(true);
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        long scriptId = getIntent().getLongExtra("script_id", -1);
        mScript = ScriptManager.getInstance().findScriptById(scriptId);
        if (mScript == null) {
            ToastUtil.show(this, R.string.invalid_script_id);
            finish();
            return;
        }

        String configName = getIntent().getStringExtra("config_name");
        Bundle bundle = new Bundle();
        bundle.putString("script_path", mScript.getScriptDir());
        bundle.putString("config_name", configName);
        bundle.putString("option_view_file", mScript.getOptionViewPath());

        Fragment optionFragment;
        if (mScript.hasOptionView()) {
            optionFragment = new HtmlOptionFragment();
        } else {
            optionFragment = new ListOptionFragment();
        }
        optionFragment.setArguments(bundle);
        getSupportFragmentManager().beginTransaction().replace(R.id.fragment_option, optionFragment).commit();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        if (mScript.hasOptionView()) {
            menu.add(Menu.NONE, R.id.action_save, 1, R.string.save_option)
                    .setIcon(R.drawable.ic_save_black_24dp)
                    .setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                finish();
                return true;
            case R.id.action_save:
                saveOption();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void saveOption() {
        Fragment current = getSupportFragmentManager().findFragmentById(R.id.fragment_option);
        if (current instanceof HtmlOptionFragment) {
            ((HtmlOptionFragment) current).onSaveConfigs();
            finish();
        }
    }
}
