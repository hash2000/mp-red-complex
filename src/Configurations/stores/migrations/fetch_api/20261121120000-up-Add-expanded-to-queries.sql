-- Признак «узел всегда открыт»: при загрузке дерева такой узел раскрывается
-- сразу вместе с вложенными узлами.
ALTER TABLE "queries" ADD COLUMN "expanded" INTEGER NOT NULL DEFAULT 0;
