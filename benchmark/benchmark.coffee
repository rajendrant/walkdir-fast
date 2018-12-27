fs = require 'fs'
path = require 'path'
walkdir = require '../'

mkdirp = (dir) ->
  return if fs.existsSync dir
  curr = ''
  for d in dir.split(path.sep)
    curr += d + path.sep
    if !fs.existsSync(curr)
      fs.mkdirSync(curr)

createFilesForTest = ->
  console.time 'createFilesForTest'
  files = 1
  for line in fs.readFileSync('benchmark/data-large.txt', 'utf8').split('\n')
    continue unless line
    line = 'benchmark/data/' + line
    mkdirp path.dirname(line)
    fs.writeFileSync line, ''
    files += 1
  console.timeEnd 'createFilesForTest'
  console.log "created #{files} files"

createFilesForTest()
console.time 'walkdir'
console.log walkdir.sync('benchmark/data/').length
console.timeEnd 'walkdir'

console.time 'walkdir'
walkdir 'benchmark/data/', ignored_globs: ['LICENSE', 'define*']
.on 'end', (files) ->
  console.timeEnd 'walkdir'
  console.log files.length
