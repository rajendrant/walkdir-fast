path = require 'path'
assert = require 'assert'
walkdir = require '../'
mm = require('micromatch')
util = require './util'

test = (dir, options={}) ->
  actual = walkdir.sync dir, JSON.parse(JSON.stringify(options))
  expected = util.walkdirExpected dir, options
  actual.sort()
  expected.sort()
  it JSON.stringify(options), ->
    assert.deepEqual actual, expected, 'test failed for dir and options'

util.createFilesForTest()

describe 'test the files returned are the same with the javascript version', ->
  describe 'with various glob pattern options', ->
    testpath = path.join 'benchmark', 'data', 'chrome'
    test testpath + path.sep + 'browser'
    test testpath + path.sep + 'browser' + path.sep
    test testpath, ignored_globs: ['DEPS']
    test testpath, ignored_globs: ['DEPS', 'OWNERS']
    test testpath, ignored_globs: ['DEPS', 'test']
    test testpath, ignored_globs: ['*.cc', '*.h']
    test testpath, ignored_globs: ['BUILD.gn']
    test testpath, ignored_globs: ['**'+path.sep+'test'+path.sep+'base'+path.sep+'**']
    test testpath, ignored_globs: ['**'+path.sep+'test'+path.sep+'!(base)'+path.sep+'**']
