package com.rainman.asf.script;

import android.content.Context;
import android.net.Uri;

import com.rainman.asf.core.ScriptEnvironment;
import com.rainman.asf.core.ScriptManager;
import com.rainman.asf.core.database.Script;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.Objects;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class ScriptImportHelper {

    private final Context mContext;
    private final ScriptManager mScriptManager;

    public ScriptImportHelper(Context context) {
        mContext = context;
        mScriptManager = ScriptManager.getInstance();
    }

    private String makeScriptDir(String guid) {
        File dir = new File(ScriptEnvironment.getScriptRootDir(), guid);
        if (!dir.exists() && !dir.mkdir())
            return null;
        return dir.getAbsolutePath();
    }

    private Integer parseVersionName(String versionName) {
        Pattern pattern = Pattern.compile("^(\\d+)\\.(\\d+)\\.(\\d+)$");
        Matcher matcher = pattern.matcher(versionName);
        if (!matcher.find()) {
            return null;
        }

        String s1 = matcher.group(1);
        String s2 = matcher.group(2);
        String s3 = matcher.group(3);
        int majorVer = s1 == null ? 0 : Integer.parseInt(s1);
        int minorVer = s2 == null ? 0 : Integer.parseInt(s2);
        int patchVer = s3 == null ? 0 : Integer.parseInt(s3);
        return (majorVer << 16) | (minorVer << 8) | patchVer;
    }

    public boolean importLocalScript(final Uri scriptUri) {
        // 解析manifest.xml文件
        ScriptFileParser parser = new ScriptFileParser(new ScriptFileParser.ScriptInputStream() {
            @Override
            public InputStream getInputStream() throws FileNotFoundException {
                if (Objects.equals(scriptUri.getScheme(), "content")) {
                    return mContext.getContentResolver().openInputStream(scriptUri);
                } else {
                    String path = scriptUri.getPath();
                    return new FileInputStream(path);
                }
            }
        });
        return importScript(parser, null);
    }

    public boolean importNetScript(final File scriptFile, String updateUrl) {
        // 解析manifest.xml文件
        ScriptFileParser parser = new ScriptFileParser(new ScriptFileParser.ScriptInputStream() {
            @Override
            public InputStream getInputStream() throws FileNotFoundException {
                return new FileInputStream(scriptFile);
            }
        });
        return importScript(parser, updateUrl);
    }

    private boolean importScript(ScriptFileParser parser, String updateUrl) {
        ScriptInfo scriptInfo = parser.parseScriptFile();
        if (scriptInfo == null)
            return false;

        if (mScriptManager.findScriptByGuid(scriptInfo.getGuid()) != null)
            return false;

        // 创建脚本存放目录
        String scriptDir = makeScriptDir(scriptInfo.getGuid());
        if (scriptDir == null)
            return false;

        // 将脚本资源文件释放到脚本目录中
        if (!parser.writeAllResFiles(scriptDir))
            return false;

        Integer versionCode = parseVersionName(scriptInfo.getVersion());
        if (versionCode == null)
            return false;

        // 将脚本记录添加到数据库中
        Script script = new Script(
                scriptInfo.getIconFile(),
                scriptInfo.getName(),
                scriptInfo.getDescription(),
                scriptInfo.getMainFile(),
                scriptDir,
                scriptInfo.getGuid(),
                versionCode,
                updateUrl,
                scriptInfo.getOptionViewFile());

        if (scriptInfo.getIconFile().isEmpty() && new File(scriptDir, "icon.png").exists()) {
            script.setIconFile("icon.png");
        }

        mScriptManager.addScript(script);
        return true;
    }

    public boolean updateLocalScript(long scriptId, final Uri scriptUri) {
        // 解析manifest.xml文件
        ScriptFileParser parser = new ScriptFileParser(new ScriptFileParser.ScriptInputStream() {
            @Override
            public InputStream getInputStream() throws FileNotFoundException {
                if (Objects.equals(scriptUri.getScheme(), "content")) {
                    return mContext.getContentResolver().openInputStream(scriptUri);
                } else {
                    String path = scriptUri.getPath();
                    return new FileInputStream(path);
                }
            }
        });
        return updateScript(scriptId, parser);
    }

    public boolean updateNetScript(long scriptId, final File scriptFile) {
        // 解析manifest.xml文件
        ScriptFileParser parser = new ScriptFileParser(new ScriptFileParser.ScriptInputStream() {
            @Override
            public InputStream getInputStream() throws FileNotFoundException {
                return new FileInputStream(scriptFile);
            }
        });
        return updateScript(scriptId, parser);
    }

    private boolean updateScript(long scriptId, ScriptFileParser parser) {
        ScriptInfo scriptInfo = parser.parseScriptFile();
        if (scriptInfo == null)
            return false;

        Script script = mScriptManager.findScriptById(scriptId);
        if (script == null)
            return false;

        // 将脚本资源文件释放到脚本目录中
        if (!parser.writeAllResFiles(script.getScriptDir()))
            return false;

        Integer versionCode = parseVersionName(scriptInfo.getVersion());
        if (versionCode == null)
            return false;

        script.setIconFile(scriptInfo.getIconFile());
        script.setName(scriptInfo.getName());
        script.setDescription(scriptInfo.getDescription());
        script.setMainFile(scriptInfo.getMainFile());
        script.setVersionCode(versionCode);
        script.setOptionViewFile(scriptInfo.getOptionViewFile());

        if (scriptInfo.getIconFile().isEmpty() && new File(script.getScriptDir(), "icon.png").exists()) {
            script.setIconFile("icon.png");
        }

        // 更新数据库中的脚本记录
        mScriptManager.updateScript(script);
        return true;
    }
}
