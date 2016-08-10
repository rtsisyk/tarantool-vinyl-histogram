#!/usr/bin/env tarantool

os.execute("rm -rf *.snap* *.xlog*")

box.cfg {
    slab_alloc_arena = 0.1,
    wal_mode = "none",
    snap_dir = ".",
    work_dir = ".";
    rows_per_wal = 100000000;
}

box.once("bootstrap", function()
    box.schema.space.create("vinyl", { engine = 'vinyl' })
    box.space.vinyl:create_index('primary', { parts = { 1, 'unsigned' } })
    box.snapshot()
end)

local fiber = require('fiber')
local clock = require('clock')
local stats = fiber.create(function()
    local time = clock.realtime()
    while true do
        print('stats', clock.realtime() - time, require('yaml').encode(box.stat().UPSERT))
        time = clock.realtime()
        fiber.sleep(1)
    end
end)

--[[
local selects = fiber.create(function()
    fiber.sleep(0)
    while true do
        box.space.vinyl:select({math.random(2000000)}, { limit = 20 })
        fiber.sleep(0.01)
    end
end)

--local cpuprof = require('gperftools.cpu')
--cpuprof.start('bench.prof')
--]]

local expirationd = require('expirationd')
local function garage_info_is_expired(args, tuple)
    return tuple[2] > 1
end

expirationd.start('garage_dev_ids', box.space.vinyl.id, garage_info_is_expired, {
    process_expired_tuple = nil,
    tuples_per_iter = 128,
    full_scan_time = 86400,
    args = nil,
})


local bench = require('cbench.bench')
bench.upserts_uint()
stats:cancel()
if selects ~= nil then
    selects:cancel()
end
 
--cpuprof.flush()

os.exit(0)
