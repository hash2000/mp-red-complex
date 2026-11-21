-- Признак «узел всегда открыт»: при загрузке дерева такой узел раскрывается
-- сразу вместе с вложенными узлами.
ALTER TABLE "nodes" ADD COLUMN "expanded" INTEGER NOT NULL DEFAULT 0;
