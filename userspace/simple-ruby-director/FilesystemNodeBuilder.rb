require 'Node'
require 'Manager'

#This class is responsible for initial parsing of node information from 
#the filesystem (at the director startup-time)
class FilesystemNodeBuilder
    
  #Checks, if core node manager exists, and if so it all its current connections are parsed
  #Used for initial state read on startup
  def parseCoreManager (filesystemConnector, nodeRepository)
      $log.debug "Parsing core managers"
      return nil if !filesystemConnector.checkForCoreManager
      $log.debug "Core manager present"
      parseCoreNodeManager(filesystemConnector, nodeRepository, CoreNodeManager.new)
  end
  
  #Reads all currently connected detached managers
  #Used for initial state read on startup
  def parseDetachedManagers (filesystemConnector, nodeRepository)
      $log.debug "Parsing detached managers"
      return nil if !filesystemConnector.checkForDetachedManager
      $log.debug "Detached manager present"
      parseDetachedManagerNodes(filesystemConnector, nodeRepository)
  end
    
  private
  
  def parseCoreNodeManager(filesystemConnector, nodeRepository, coreNodeManager)
      filesystemConnector.forEachCoreManagerNodeDir() { |slotIndex, fullFileName|
          File.open("#{fullFileName}/connections/ctrlconn/peername", "r") { |aFile|
              $log.debug "parsing core manager node at index #{slotIndex}"
              readData = aFile.readline("\0")
              ipAddress = readData.split(":").first  
              #node, isNew = nodeRepository.getOrCreateNode(ipAddress, ipAddress)
	      placeHolderNode = Node.new(nil, ipAddress) # Just a placeholder node with no id, not even registered to repository
              $log.debug "registering node #{ipAddress}"
              coreNodeManager.registerDetachedNode(slotIndex.to_i, placeHolderNode)
          }                    
      }
      coreNodeManager
  end


  def parseDetachedManagerNodes(filesystemConnector, nodeRepository)
      detachedManagers = []
      filesystemConnector.forEachDetachedManagerDir() { |slotIndex, fullFileName|
          File.open("#{fullFileName}/connections/ctrlconn/peername", "r") { |aFile|
              $log.debug "parsing detached manager node at index #{slotIndex}"
              readData = aFile.readline("\0")
              ipAddress = readData.split(":").first  
              #node, isNew = nodeRepository.getOrCreateNode(ipAddress, ipAddress)
	      placeHolderNode = Node.new(nil, ipAddress) # Just a placeholder node with no id, not even registered to repository
              $log.debug "registering node #{placeHolderNode} from #{ipAddress}."
              detachedManagers[slotIndex.to_i] = DetachedNodeManager.new(placeHolderNode, slotIndex.to_i)
          }
      }
      detachedManagers
  end
  
end