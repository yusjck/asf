package com.rainman.asf.core.database;

import androidx.room.*;

import java.util.List;

@Dao
public interface VisitorDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    long addOrUpdate(Visitor data);

    @Delete
    void delete(Visitor data);

    @Query("DELETE FROM `Visitor` WHERE `id`=:id")
    void delete(long id);

    @Query("SELECT * FROM `Visitor` WHERE `name`=:name AND `signature`=:signature")
    Visitor findVisitor(String name, String signature);

    @Query("SELECT * FROM `Visitor`")
    List<Visitor> getAll();
}
