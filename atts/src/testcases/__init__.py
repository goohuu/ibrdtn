import base0, filetransfer, faketest, dtnping, poettner

""" define all testcases """

testlist = []
#testlist.append( base0.ConnectionTest() )
testlist.append( poettner.SummaryReport() )
testlist.append( poettner.MemoryReport() )
testlist.append( poettner.ThroughputReport() )

#testlist.append( base0.UpdateSoftware() )
#testlist.append( filetransfer.SimpleFileTransfer() )
#testlist.append( filetransfer.WiredFileTransfer() )
#testlist.append( filetransfer.EpidemicFileTransfer() )
#testlist.append( filetransfer.VPNFileTransfer() )
#testlist.append( dtnping.SimplePing() )

#testlist.append( faketest.FakeTestFailed() )
#testlist.append( faketest.FakeTestSuccessful() )

