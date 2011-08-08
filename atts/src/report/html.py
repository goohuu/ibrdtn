'''
Created on 17.06.2010

@author: morgenro
'''

from report.base import BasicReport
import os

class HTMLReport(BasicReport):
    
    def __init__(self, datadir, reportables):
        BasicReport.__init__(self, reportables)
        self.datadir = datadir
    
    def gen(self):
        """ create the report """
        f = open(self.datadir + "/index.html", 'w')
        
        f.write("<html>\n")
        f.write("  <head>\n")
        f.write("    <link rel='stylesheet' media='screen,projection' type='text/css' href='style.css' />\n")
        f.write("    <title>ATTS Report</title>\n")
        f.write("  </head>\n")
        f.write("  <body>\n")
        f.write("    <p id='banner'><img src='logo.png' alt='Logo' id='logo' /><h1>ATTS Report</h1></p>\n")
        f.write("    <div id='content'>\n")
        

        self.writeSummary(f)
        
        for r in self.reports:
            self.writeDetailedReport(f, r)
            
        f.write("  </div></body>\n")
        f.write("</html>\n")
        
        f.close()
        
    def writeSummary(self, f):
        
        f.write("<table>\n")
        f.write("<tr>\n")
        f.write("<th>Name</th><th>Status</th>\n")
        f.write("</tr>\n")

        for r in self.reports:
            f.write("<tr>\n")
            f.write("<td><a href='#" + r.name + "'>" + r.name + "</a></td>")
            
            if r.fail:
                f.write("<td><span class='signalred'>failed</span></td>\n")
            else:
                f.write("<td><span class='green'>successful</span></td>\n")
                
            f.write("</tr>\n")
            
        f.write("</table>\n")

    def writeDetailedReport(self, f, report):
        f.write("<h2><a name='" + report.name + "'>" + report.name + "</a></h2>\n")
        f.write("<p>Status: ")
        if report.fail:
            f.write("<span class='signalred'>failed</span>")
        else:
            f.write("<span class='green'>successful</span>")
        f.write("</p>\n")
        f.write("<p class='description'>" + report.description + "</p>\n")
        
        """ create all graphs """
        for g in report.graphs:
            f.write("<h3>Graph: <span class='blue'>" + g.name + "</span></h3>\n")
            f.write("<p class='description'>" + g.description + "</p>\n")
            fileprefix = self.datadir + "/" + report.name + "-" + str(g.identifier)
            plotimage = g.plot(fileprefix)
            f.write("<p><img src='" + os.path.basename(plotimage) + "' tag='" + os.path.basename(fileprefix) + "' /></p>\n")
        
        """ write text output """
        if report.text != None:
            f.write("<h3>Output</h3>\n")
            f.write("<pre>")
            f.write(report.text);
            f.write("</pre>\n")
        
        """ write additional files """
        self.writeFiles(f, report)
    
    def writeFiles(self, f, report):
        f.write("<h3>Additional files</h3>\n")
        f.write("<div><ul>")
        
        max_report = len(report.reportfiles)
        for i in range(0, max_report):
            outfile = os.path.basename(self.writeFile(report.reportfiles[i], report))
            
            f.write("<li><a href='" + outfile + "'>" + report.reportfiles[i].name + "</a> (" + report.reportfiles[i].hostname + ")</li>")
            #if (max_report - 1) > i:
            #    f.write(", ")
            
        f.write("</ul></div>\n")
            
    def writeFile(self, rfile, report):
        outfile = self.datadir + "/" + os.path.basename(rfile.file) + ".html"
        f = open(outfile, 'w')
        
        f.write("<html>\n")
        f.write("  <head>\n")
        f.write("    <link rel='stylesheet' media='screen,projection' type='text/css' href='style.css' />\n")
        f.write("    <title>ATTS Report</title>\n")
        f.write("  </head>\n")
        f.write("  <body>\n")
        f.write("    <p id='banner'><img src='logo.png' alt='Logo' id='logo' /><h1>ATTS Report</h1></p>\n")
        f.write("    <div id='content'>\n")

        f.write("<h2><a name='" + report.name + "'>" + report.name + "</a></h2>\n")
        
        """ write text output """
        f.write("<h3>Filename: " + rfile.name + ", Host: " + rfile.hostname + "</h3>\n")
        
        textfile = open(rfile.file, 'r')
        
        f.write("<pre>")
        for line in textfile.readlines():
            f.write(line);
            
        f.write("</pre>\n")
        
        textfile.close()
            
        f.write("  </div></body>\n")
        f.write("</html>\n")
        
        f.close()
        
        return outfile
