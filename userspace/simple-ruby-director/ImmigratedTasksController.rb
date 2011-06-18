# Class keeping track of all immigrated tasks
#
# TODO: Make thread safe
class ImmigratedTasksController
  def initialize()
    @immigratedTasks = {}
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
end