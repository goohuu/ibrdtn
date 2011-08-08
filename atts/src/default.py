#!/usr/bin/python
#
# run ATTS
# (c) 2010 IBR

import sys
import ConfigParser
from optparse import OptionParser
from testcases import testlist
import traceback
from report.html import HTMLReport 

if __name__ == '__main__':
    # print a some starting information
    print("- ATTS 0.1 -")
    
    # read parameters
    usage = "usage: %prog [options] <testcase>"
    parser = OptionParser(usage=usage)
    parser.add_option("-c", "--config", dest="file", default="data/default.ini",
    help="Set the configuration file for this run.")
    
#    parser.add_option("-s", "--setup", action="store_true", dest="setup",
#                      help="Do a prototype setup before the simulation starts.")
#    parser.add_option("-r", "--run", action="store_true", dest="run",
#                      help="Run the simulation.")
#    parser.add_option("-t", "--tmp-dir", dest="tmpdir", default="/tmp",
#        help="define a temporary directory")
#    parser.add_option("-d", "--data-dir", dest="datadir", default="../data",
#        help="specify the data directory containing setup scripts and ssh keys")

    # parse arguments
    (options, args) = parser.parse_args()

    # read configuration
    config = ConfigParser.RawConfigParser()
    print("read configuration: " + options.file)
    config.read(options.file);
    
    if len(args) > 0:
        (mod, case) = args[0].split(".")
        exec("from testcases import " + mod)
        exec("tests = [ " + args[0] + "() ]")
    else:
        tests = testlist
    
    for t in tests:
        try:
            print("testcase: " + t.name + " ... [prepare]")
            t.prepare()
            print("testcase: " + t.name + " ... [run]")
            t.run()
        except:
            traceback.print_exc(file=sys.stdout)
            t.fail = True
        finally:
            print("testcase: " + t.name + " ... [cleanup]")
            t.cleanup()

        if t.fail:
            print("testcase: " + t.name + " => FAILED")
        else:
            print("testcase: " + t.name + " => SUCCESSFUL")
            
    """ create a data report in HTML """
    reportdir = "data/htdocs"
    report = HTMLReport(reportdir, tests)
    report.gen()
    
    