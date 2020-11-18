package com.rainman.asf.core.database;

import androidx.sqlite.db.SupportSQLiteDatabase;
import androidx.room.Database;
import androidx.room.Room;
import androidx.room.RoomDatabase;
import androidx.room.migration.Migration;

import android.content.Context;

@Database(entities = {Script.class, ScriptLog.class, Scheduler.class, Visitor.class}, version = 5, exportSchema = false)
public abstract class CoreDatabase extends RoomDatabase {

    private static final String DB_NAME = "coredata.db";
    private static volatile CoreDatabase mCoreDataBase;

    public abstract ScriptDao getScriptDao();

    public abstract ScriptLogDao getScriptLogDao();

    public abstract SchedulerDao getSchedulerDao();

    public abstract VisitorDao getVisitorDao();

    private static final Migration MIGRATION_1_2 = new Migration(1, 2) {
        @Override
        public void migrate(SupportSQLiteDatabase database) {
            database.execSQL("ALTER TABLE `Script` ADD COLUMN `guid` TEXT");
            database.execSQL("ALTER TABLE `Script` ADD COLUMN `versionCode` INTEGER NOT NULL DEFAULT 1");
            database.execSQL("ALTER TABLE `Script` ADD COLUMN `updateUrl` TEXT");
        }
    };

    private static final Migration MIGRATION_2_3 = new Migration(2, 3) {
        @Override
        public void migrate(SupportSQLiteDatabase database) {
            database.execSQL("CREATE TABLE `Visitor` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `name` TEXT, `signature` TEXT, `accessPermission` INTEGER NOT NULL, `expireTime` INTEGER NOT NULL, `lastAccessTime` INTEGER NOT NULL)");
        }
    };

    private static final Migration MIGRATION_3_4 = new Migration(3, 4) {
        @Override
        public void migrate(SupportSQLiteDatabase database) {
            database.execSQL("ALTER TABLE `Script` ADD COLUMN `lastRunTime` INTEGER NOT NULL DEFAULT 0");
            database.execSQL("ALTER TABLE `Script` ADD COLUMN `lastQuitReason` TEXT");
        }
    };

    private static final Migration MIGRATION_4_5 = new Migration(4, 5) {
        @Override
        public void migrate(SupportSQLiteDatabase database) {
            database.execSQL("ALTER TABLE `Script` ADD COLUMN `optionView` TEXT");
            database.execSQL("ALTER TABLE `Scheduler` ADD COLUMN `configName` TEXT");
            database.execSQL("ALTER TABLE `Scheduler` ADD COLUMN `configEnabled` INTEGER NOT NULL DEFAULT 0");
        }
    };

    public static synchronized CoreDatabase getInstance(Context context) {
        if (mCoreDataBase == null) {
            mCoreDataBase = Room.databaseBuilder(context, CoreDatabase.class, DB_NAME)
                    .allowMainThreadQueries()
                    .addMigrations(MIGRATION_1_2)
                    .addMigrations(MIGRATION_2_3)
                    .addMigrations(MIGRATION_3_4)
                    .addMigrations(MIGRATION_4_5)
                    .build();
        }
        return mCoreDataBase;
    }
}
