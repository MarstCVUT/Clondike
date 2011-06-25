require 'measure/MeasurementPlan.rb'

#SAMPLE CONFIG
#
#Nodes count: 
#  4
#
#Tasks:
#  ls: exec "ls -la" /tmp
#  gcc: exec "gcc a.cpp -o a.out" /home/user/test
#  gcc2: exec "gcc b.cpp -o a.out" /home/user/test
#
#Nodes: 
#  LocalNode: ls
#  RemoteNode1, RemoteNode2: gcc 30
#  ALL: gcc2 60

class MeasurementPlanParser
  # Delay how long to wait before starting to get remote nodes time to block accepting of events
  SAFETY_DELAY_FOR_BLOCKING = 5
  
  def initialize(nodeRepository)
    @nodeRepository = nodeRepository   
  end
  
  def initializeMeasurement(fileName, startTime, outputFileName)      
      $log.debug("Parsing measurement plan #{fileName}. Will output result to #{outputFileName}")
      @commands = {}
      measurement = Measurement.new(startTime + SAFETY_DELAY_FOR_BLOCKING, outputFileName)
      
      # Will read all lines in memory.. no issues since we do not assume large files
      lines = []      
      IO.foreach(fileName) { |line|
	lines << line
      }
      
      lines = lines.find_all { |line| !line.startsWith("#") and line.chop.length > 0 }
            
      raise "Config file should not be empty" if ( lines.empty? )
      
      buildNodeMapping(measurement, lines)      
      parseTasks(measurement, lines)
      parseNodes(measurement, lines)
      
      return measurement
  end
  
private  
  def buildNodeMapping(measurement, lines)
    if lines[0] =~ /^Nodes count/
      lines.shift
      line = lines.shift
      nodeCount = line.gsub(/\n/,'').to_i
      
      preferredBinding = parseStaticBinding(measurement, lines)      
      measurement.buildNodeMapping(@nodeRepository, preferredBinding, nodeCount)
    else
      preferredBinding = parseStaticBinding(measurement, lines)
      measurement.buildNodeMappingForAllKnownNodes(@nodeRepository, preferredBinding)
    end
   end

   def parseStaticBinding(measurement, lines)      
     preferredBinding = {}
     if ( lines[0] =~ /^Bind/ )
       lines.shift
       line = lines.shift
        while ( !lines.empty? and !line.strip.empty? )
	  groups = line.match(/([^:]+): ([\d\.]+)/)  
	  break if !groups 
	  nodeName = groups[1].strip
	  nodeIp = groups[2]
	  preferredBinding[nodeIp] = nodeName

	  line = lines.shift
	end       
     end
     
     return preferredBinding
   end

   def parseTasks(measurement, lines)      
     while ( !lines.empty?  )
       line = lines.shift
       
       # Check for nodes section
       if ( line =~ /^Nodes/ )
	 break
       end
       
       # Check for match for task line syntax and parse it
       groups = line.match(/([^:]+): ([^\W]+.*)/)
       if ( groups )	 
	  taskName = groups[1].strip
	  taskCommand = groups[2]
	  raise "Duplicate task name: #{taskName}" if @commands.has_key?(taskName) 
	  @commands[taskName] = parseCommand(taskCommand)
       end
     end          
   end
   
   def parseNodes(measurement, lines)            
     while ( !lines.empty?  )
       line = lines.shift
       
       # Check for match for task line syntax and parse it
       groups = line.match(/([^:]+): (.*)/)
       if ( groups )
	  nodes = groups[1]
	  task = groups[2]
	  
	  associateNodesWithTask(measurement, nodes, task)
       end
     end                 
   end
   
   def parseCommand(command)
     if ( command.startsWith("exec") )
       #puts "CMD: #{command}" 
       groups = command.match(/exec \"(.*)\" (.*)/)
       execCommand = groups[1]
       execPath = groups[2]
       return ExecuteCommand.new(execCommand, execPath)
     else
       raise "Unknown command: #{command}"
     end
   end
   
   # Returns node list or nil in case all nodes should match
   def parseTaskNodes(nodes)
     return nil if ( nodes.include?("ALL") )
     return nodes.split(',')
   end
   
   def parseTask(taskString)
     parts = taskString.split(' ')
     taskName = parts[0].strip
     tastStartOffeset =  parts.length == 2 ? parts[1].to_f : 0
     return MeasurementTask.new(@commands[taskName.strip], tastStartOffeset)
   end
   
   def associateNodesWithTask(measurement, nodesString, taskString)
     nodes = parseTaskNodes(nodesString)
     task = parseTask(taskString)
     if ( nodes == nil )
       measurement.addAllNodesTaskAssignement(task)
     else
       nodes.each { |node|
	  measurement.addNodeTaskAssignement(node.strip, task)
       }
     end
   end      
end