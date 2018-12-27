fs = require 'fs'
path = require 'path'
assert = require 'assert'
walkdir = require '../'
mm = require('micromatch')

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
  for line in fs.readFileSync(path.join('benchmark', 'data-large.txt'), 'utf8').split('\n')
    continue unless line
    continue unless line.startsWith 'chrome'
    line = 'benchmark/data/' + line
    line = path.join ...line.split('/')
    mkdirp path.dirname(line)
    fs.writeFileSync line, ''
    files += 1
  console.timeEnd 'createFilesForTest'
  console.log "created #{files} files"

walkdirExpected = (dir, options, filelist=[]) ->
  dir = dir + path.sep if dir.slice(-1)!=path.sep
  for file in fs.readdirSync(dir)
    fullname = dir + file
    if options.ignored_globs?
      continue if mm.any file, options.ignored_globs, {matchBase: true, dot: true}
      continue if mm.any fullname, options.ignored_globs, {matchBase: true, dot: true}
    if fs.statSync(fullname).isDirectory()
      filelist = walkdirExpected(fullname + path.sep, options, filelist);
    else
      filelist.push(fullname);
  return filelist

test = (dir, options={}) ->
  actual = walkdir.sync dir, JSON.parse(JSON.stringify(options))
  expected = walkdirExpected dir, options
  actual.sort()
  expected.sort()
  it JSON.stringify(options), ->
    assert.deepEqual actual, expected, 'test failed for dir and options'

createFilesForTest()

describe 'test the files returned are the same with the javascript version', ->
  describe 'with various glob pattern options', ->
    testpath = path.join 'benchmark', 'data', 'chrome'
    test testpath + path.sep + 'browser'
    test testpath + path.sep + 'browser' + path.sep
    test testpath, ignored_globs: ['DEPS']
    test testpath, ignored_globs: ['DEPS', 'OWNERS']
    test testpath, ignored_globs: ['DEPS', 'test']
    test testpath, ignored_globs: ['*.cc', '*.h']
    test testpath, ignored_globs: ['**'+path.sep+'test'+path.sep+'base'+path.sep+'**']
    test testpath, ignored_globs: ['**'+path.sep+'test'+path.sep+'!(base)'+path.sep+'**']
