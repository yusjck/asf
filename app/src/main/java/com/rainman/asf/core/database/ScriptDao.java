package com.rainman.asf.core.database;

import androidx.room.*;

import java.util.List;

@Dao
public interface ScriptDao {

    @Insert
    long insert(Script data);

    @Update
    void update(Script data);

    @Delete
    void delete(Script data);

    @Query("delete from Script where id=:id")
    void delete(long id);

    @Query("select * from Script")
    List<Script> getAll();

    @Query("select * from Script where id=:id")
    Script findById(long id);
}
