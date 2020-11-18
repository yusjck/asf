package com.rainman.asf.core.database;

import androidx.room.*;

import java.util.List;

@Dao
public interface ScriptLogDao {

    @Insert
    long insert(ScriptLog data);

    @Query("select count(*) from ScriptLog")
    int getCount();

    @Query("select * from ScriptLog limit :start, :num")
    List<ScriptLog> getPage(int start, int num);

    @Query("select * from ScriptLog")
    List<ScriptLog> getAll();

    @Query("delete from ScriptLog")
    void deleteAll();

    @Query("select max(id) from ScriptLog")
    long getMaxId();
}
