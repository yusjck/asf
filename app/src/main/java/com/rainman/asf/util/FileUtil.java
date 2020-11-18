package com.rainman.asf.util;

import java.io.*;
import java.nio.channels.FileChannel;

public class FileUtil {

    /**
     * 文件复制
     *
     * @param sourcePath 源文件路径
     * @param targetPath 复制后存放路径
     * @throws Exception
     */
    public static void copyFile(String sourcePath, String targetPath) throws Exception {
        File source = new File(sourcePath);
        if (!source.exists()) {
            throw new Exception("file not exist");
        }
        if (!source.isFile()) {
            throw new Exception("not a file");
        }

        File target = new File(targetPath);
        if (target.isDirectory()) {
            // 获取源文件的文件名
            String fileName = sourcePath.substring(sourcePath.lastIndexOf(File.separator) + 1);
            target = new File(targetPath, fileName);
        }

        FileChannel inputChannel = null;
        FileChannel outputChannel = null;
        try {
            inputChannel = new FileInputStream(source).getChannel();
            outputChannel = new FileOutputStream(target).getChannel();
            outputChannel.transferFrom(inputChannel, 0, inputChannel.size());
        } finally {
            if (inputChannel != null) {
                inputChannel.close();
            }
            if (outputChannel != null) {
                outputChannel.close();
            }
        }
    }

    public static String readTextFile(File sourceFile) {
        InputStreamReader reader = null;
        StringBuilder stringBuilder = new StringBuilder();
        try {
            reader = new InputStreamReader(new FileInputStream(sourceFile));
            char[] buf = new char[1024];
            int len;
            while ((len = reader.read(buf)) != -1) {
                stringBuilder.append(buf, 0, len);
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return stringBuilder.toString();
    }

    public static void writeTextFile(File targetFile, String content) {
        OutputStreamWriter writer = null;
        try {
            writer = new OutputStreamWriter(new FileOutputStream(targetFile));
            writer.write(content);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (writer != null) {
                try {
                    writer.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public static byte[] readFile(File sourceFile) {
        FileInputStream inputStream = null;
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        try {
            inputStream = new FileInputStream(sourceFile);
            byte[] buf = new byte[4096];
            int len;
            while ((len = inputStream.read(buf, 0, 4096)) != -1) {
                outputStream.write(buf, 0, len);
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        } catch (IOException e) {
            e.printStackTrace();
            return null;
        } finally {
            if (inputStream != null) {
                try {
                    inputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return outputStream.toByteArray();
    }

    public static boolean writeFile(File targetFile, byte[] content) {
        FileOutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream(targetFile);
            outputStream.write(content);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return false;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        } finally {
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

    public static boolean deleteFile(File file) {
        if (!file.exists()) {
            return false;
        }

        if (file.isDirectory()) {
            File[] files = file.listFiles();
            for (File f : files) {
                deleteFile(f);
            }
        }
        return file.delete();
    }
}
