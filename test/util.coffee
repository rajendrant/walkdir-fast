fs = require 'fs'
path = require 'path'
mm = require('micromatch')

mkdirp = (dir) ->
  return if fs.existsSync dir
  curr = ''
  for d in dir.split(path.sep)
    curr += d + path.sep
    if !fs.existsSync(curr)
      fs.mkdirSync(curr)

walkdirExpected = (dir, options, filelist) ->
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

module.exports =
  createFilesForTest: (startpath='') ->
    console.time 'createFilesForTest'
    files = 1
    for line in fs.readFileSync(path.join('benchmark', 'data-large.txt'), 'utf8').split('\n')
      continue unless line
      continue unless line.startsWith startpath
      line = 'benchmark/data/' + line
      line = path.join ...line.split('/')
      mkdirp path.dirname(line)
      fs.writeFileSync line, ''
      files += 1
    console.timeEnd 'createFilesForTest'
    console.log "created #{files} files"

  walkdirExpected: (dir, options, filelist=[]) ->
    walkdirExpected(dir, options, filelist)
