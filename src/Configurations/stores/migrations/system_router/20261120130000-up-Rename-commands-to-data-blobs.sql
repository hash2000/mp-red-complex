ALTER TABLE "commands" RENAME TO "data_blobs";
ALTER TABLE "data_blobs" RENAME COLUMN "script" TO "data";
