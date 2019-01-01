path = require 'path'
walkdir = require '../'
util = require '../test/util'
assert = require 'assert'

util.createFilesForTest()

test = (dir, options={}) ->
  console.time 'walkdir-fast'
  actual = walkdir.sync dir, options
  console.timeEnd 'walkdir-fast'
  console.time 'nodejs-fs'
  expected = util.walkdirExpected dir, options
  console.timeEnd 'nodejs-fs'
  assert expected, actual
  console.log expected.length, actual.length

testpath = path.join 'benchmark', 'data'
test testpath
test testpath, ignored_globs: ['LICENSE', 'define*']

console.time 'walkdir-fast-async'
walkdir 'benchmark/data/', ignored_globs: ['LICENSE', 'define*']
.on 'end', (files) ->
  console.timeEnd 'walkdir-fast-async'
  console.log files.length
