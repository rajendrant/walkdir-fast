walkdir = require '../'
util = require '../test/util'

util.createFilesForTest()
console.time 'walkdir'
console.log walkdir.sync('benchmark/data/').length
console.timeEnd 'walkdir'

console.time 'walkdir'
walkdir 'benchmark/data/', ignored_globs: ['LICENSE', 'define*']
.on 'end', (files) ->
  console.timeEnd 'walkdir'
  console.log files.length
