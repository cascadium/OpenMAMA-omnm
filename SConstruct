
AddOption('--with-mamasource',
          dest='with_mamasource',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='location of the uncompiled OpenMAMA Source code')

AddOption('--with-mamainstall',
          dest='with_mamainstall',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='location of a compiled OpenMAMA installation prefix.')

AddOption('--with-gtest',
          dest='with_gtest',
          type='string',
          nargs=1,
          action='store',
          metavar='DIR',
          help='location of google test framework.')

SConscript('src/SConscript', variant_dir='objects', duplicate=0)
