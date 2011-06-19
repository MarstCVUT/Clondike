# Class keeping track of all immigrated tasks
#
# TODO: Make thread safe
class ImmigratedTasksController
  def initialize(filesystemConnector)
    @immigratedTasks = {}
    @filesystemConnector = filesystemConnector
  end
  
  def onImmigrationConfirmed(node, name, localPid, remotePid)
    @immigratedTasks[localPid] = node
    #$log.debug( "Immigrated tasks count (immig) #{@immigratedTasks.size}")
  end
  
  def onExit(pid, exitCode, rusage)
    @immigratedTasks.delete(pid)
    #$log.debug( "Immigrated tasks count (exit) #{@immigratedTasks.size}")
  end
  
  def onFork(pid, parentPid)
    if ( @immigratedTasks.has_key?(parentPid) )
      @immigratedTasks[pid] = @immigratedTasks[parentPid]
    end
  end
  
  def immigratedTaskCount()
    return @immigratedTasks.size
  end
  
  def migrateAllHome()
    ExceptionAwareThread.new() {
	migrateHomeThread()
    }
  end
  
private
    def migrateHomeThread
	$log.info("Preparing to request migrate home")
	# Wait 10 seconds before migrating home just to give locally running remote tasks some chance to finish
	sleep(10)
        @immigratedTasks.each_key { |pid|
	    $log.info("Requesting migrated home for #{pid}")
	    @filesystemConnector.migrateHome(DETACHED_MANAGER_SLOT ,pid)
	}
    end

end