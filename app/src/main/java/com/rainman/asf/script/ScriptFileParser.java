package com.rainman.asf.script;

import android.util.Log;

import com.rainman.asf.util.Constant;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import java.io.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

class ScriptFileParser {

    private static final String TAG = "ScriptFileParser";
    private final ScriptInputStream mScriptInputStream;

    interface ScriptInputStream {
        InputStream getInputStream() throws FileNotFoundException;
    }

    ScriptFileParser(ScriptInputStream scriptInputStream) {
        mScriptInputStream = scriptInputStream;
    }

    private byte[] readManifestFile() {
        InputStream inputStream = null;
        try {
            inputStream = mScriptInputStream.getInputStream();
            ZipInputStream zipInputStream = new ZipInputStream(inputStream);
            for (; ; ) {
                ZipEntry zipEntry = zipInputStream.getNextEntry();
                if (zipEntry == null)
                    break;

                if (zipEntry.isDirectory())
                    continue;

                if (zipEntry.getName().equals(Constant.SCRIPT_MANIFEST_FILE)) {
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    byte[] buf = new byte[1024];
                    int len;
                    while ((len = zipInputStream.read(buf)) != -1) {
                        outputStream.write(buf, 0, len);
                    }
                    return outputStream.toByteArray();
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return null;
    }

    boolean writeAllResFiles(String scriptPath) {
        InputStream inputStream = null;
        BufferedOutputStream outputStream = null;
        try {
            inputStream = mScriptInputStream.getInputStream();
            ZipInputStream zipInputStream = new ZipInputStream(inputStream);
            for (; ; ) {
                ZipEntry zipEntry = zipInputStream.getNextEntry();
                if (zipEntry == null)
                    break;

                File newFile = new File(scriptPath, zipEntry.getName());
                Log.i(TAG, "unzipping to " + newFile.getAbsolutePath());

                if (zipEntry.isDirectory()) {
                    newFile.mkdirs();
                } else {
                    newFile.getParentFile().mkdirs();
                    outputStream = new BufferedOutputStream(new FileOutputStream(newFile));

                    byte[] buf = new byte[1024];
                    int len;
                    while ((len = zipInputStream.read(buf)) != -1) {
                        outputStream.write(buf, 0, len);
                    }

                    outputStream.close();
                    outputStream = null;
                }
                zipInputStream.closeEntry();
            }
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return true;
    }

    ScriptInfo parseScriptFile() {
        byte[] manifest = readManifestFile();
        if (manifest == null)
            return null;

        Log.i(TAG, new String(manifest));

        ScriptInfo scriptInfo = new ScriptInfo();
        ByteArrayInputStream inputStream = new ByteArrayInputStream(manifest);
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document document = builder.parse(inputStream);

            Element element = document.getDocumentElement();
            if (!element.getTagName().equals("Script"))
                return null;

            scriptInfo.setIconFile(element.getAttribute("icon"));
            scriptInfo.setName(element.getAttribute("name"));
            scriptInfo.setDescription(element.getAttribute("description"));
            scriptInfo.setMainFile(element.getAttribute("main"));
            scriptInfo.setGuid(element.getAttribute("guid"));
            scriptInfo.setVersion(element.getAttribute("version"));
            scriptInfo.setOptionViewFile(element.getAttribute("optionView"));

            if (scriptInfo.getName().isEmpty() || scriptInfo.getGuid().isEmpty())
                return null;

            NodeList fileList = element.getElementsByTagName("File");
            for (int i = 0; i < fileList.getLength(); i++) {
                Element item = (Element) fileList.item(i);
                scriptInfo.getResFileList().add(item.getTextContent());
            }
            return scriptInfo;
        } catch (ParserConfigurationException | SAXException | IOException e) {
            e.printStackTrace();
        }
        return null;
    }
}
