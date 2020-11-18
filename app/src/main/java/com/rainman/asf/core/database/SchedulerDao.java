package com.rainman.asf.core.database;

import androidx.room.*;

import java.util.List;

@Dao
public interface SchedulerDao {

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    long addOrUpdate(Scheduler data);

    @Delete
    void delete(Scheduler data);

    @Query("DELETE FROM `Scheduler` WHERE `scriptId`=:scriptId")
    int deleteByScriptId(long scriptId);

    @Query("SELECT * FROM `Scheduler` WHERE `enabled`=1")
    List<Scheduler> getEnabledSchedulers();

    @Query("SELECT * FROM `Scheduler` WHERE `id`=:id")
    Scheduler findById(long id);

    @Query("SELECT Scheduler.id,hour,minute,repeat,scriptId,configName,configEnabled,enabled,Script.name AS scriptName FROM Script INNER JOIN Scheduler ON Script.id=Scheduler.scriptId WHERE Script.id=:scriptId ORDER BY Scheduler.hour,Scheduler.minute")
    List<Scheduler.SchedulerInfo> getSchedulersByScriptId(long scriptId);

    @Query("SELECT Scheduler.id,hour,minute,repeat,scriptId,configName,configEnabled,enabled,Script.name AS scriptName FROM Script INNER JOIN Scheduler ON Script.id=Scheduler.scriptId ORDER BY Scheduler.hour,Scheduler.minute")
    List<Scheduler.SchedulerInfo> getAllSchedulers();
}
