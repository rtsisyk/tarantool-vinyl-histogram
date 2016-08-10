#!/usr/bin/env tarantool

os.execute("rm -rf *.snap* *.xlog*")

box.cfg {
    slab_alloc_arena = 0.1,
    wal_mode = "write",
    snap_dir = ".",
    work_dir = ".";
    rows_per_wal = 100000000;
}

box.once("bootstrap", function()
    box.schema.space.create("vinyl", { engine = 'vinyl' })
    box.space.vinyl:create_index('primary', { parts = { 1, 'unsigned' } })
    box.snapshot()
end)

local bench = require('cbench.bench')
bench.upserts_uint()

os.exit(0)
