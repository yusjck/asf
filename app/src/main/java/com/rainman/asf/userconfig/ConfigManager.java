package com.rainman.asf.userconfig;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;

public class ConfigManager {

    private ArrayList<ConfigGroup> groups = new ArrayList<>();
    private File mConfigDir;

    public ConfigManager() {

    }

    public ConfigManager(File configDir) {
        loadConfigs(configDir);
    }

    public ArrayList<ConfigGroup> getGroups() {
        return groups;
    }

    public File getConfigDir() {
        return mConfigDir;
    }

    public boolean loadConfigs(File configDir) {
        mConfigDir = configDir;
        groups.clear();
        FileInputStream inputStream = null;
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        try {
            inputStream = new FileInputStream(new File(configDir, "UserVarDef.xml"));
            DocumentBuilder builder = factory.newDocumentBuilder();
            Document document = builder.parse(inputStream);
            Element element = document.getDocumentElement();
            if (element.getTagName().equals("UserVars")) {
                NodeList varGroups = element.getElementsByTagName("VarGroup");
                for (int i = 0; i < varGroups.getLength(); i++) {
                    Element item = (Element) varGroups.item(i);
                    String groupName = item.getAttribute("text");
                    NodeList varItems = item.getElementsByTagName("UserVar");
                    if (!addGroup(groupName, varItems))
                        return false;
                }
                return true;
            }
        } catch (ParserConfigurationException | IOException | SAXException e) {
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
        return false;
    }

    private boolean addGroup(String groupName, NodeList subItems) {
        ConfigGroup group = new ConfigGroup();
        group.setText(groupName);
        for (int i = 0; i < subItems.getLength(); i++) {
            Element element = (Element) subItems.item(i);
            String type = element.getAttribute("type");
            switch (type) {
                case "Edit":
                    addEditItem(group, element);
                    break;
                case "List":
                case "DropList":
                    addListItem(group, element);
                    break;
                case "Switch":
                case "CheckBox":
                    addSwitchItem(group, element);
                    break;
                default:
                    return false;
            }
        }
        groups.add(group);
        return true;
    }

    private void addSwitchItem(ConfigGroup group, Element element) {
        SwitchItem item = new SwitchItem();
        item.setName(element.getAttribute("name"));
        item.setText(element.getAttribute("text"));
        item.setDescription(element.getAttribute("description"));
        item.setDefaultValue(element.getAttribute("default"));
        group.getItems().add(item);
    }

    private void addListItem(ConfigGroup group, Element element) {
        ListItem item = new ListItem();
        item.setName(element.getAttribute("name"));
        item.setText(element.getAttribute("text"));
        item.setDescription(element.getAttribute("description"));
        item.setDefaultValue(element.getAttribute("default"));
        NodeList varItems = element.getElementsByTagName("Option");
        for (int i = 0; i < varItems.getLength(); i++) {
            Element opt = (Element) varItems.item(i);
            String text = opt.getAttribute("text");
            String value = opt.getAttribute("value");
            item.getOptions().put(value, text);
        }
        group.getItems().add(item);
    }

    private void addEditItem(ConfigGroup group, Element element) {
        EditItem item = new EditItem();
        item.setName(element.getAttribute("name"));
        item.setText(element.getAttribute("text"));
        item.setDescription(element.getAttribute("description"));
        item.setDefaultValue(element.getAttribute("default"));
        if ("true".equals(element.getAttribute("mask"))) {
            item.setMask(true);
        }
        group.getItems().add(item);
    }
}
