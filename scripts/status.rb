#!/usr/bin/ruby

class Task
  attr_reader :pid
  attr_reader :cmdline
public  
  def initialize(pid, cmdline)
    @pid = pid
    @cmdline = cmdline
  end  
end

class NodeInfo
  attr_reader :index
  attr_reader :ip
  attr_reader :state
public
  def initialize(index, ip, state)
    @index = index
    @ip = ip
    @state = state
    @tasks = []
  end
  
  def addTask(task)
    @tasks << task
  end
  
  def printInfo()
    puts "#{index}: #{ip} #{state}"
    @tasks.each { |task|
      puts "   #{task.pid}: #{task.cmdline}"
    }
  end
end
  

def parseTasksData(rootPath, nodeMap)
  Dir.foreach("#{rootPath}/mig/migproc/") { |filename|      
      next if filename =~ /^\..?/ # Exclude . and ..
                                          
      taskPath = "#{rootPath}/mig/migproc/#{filename}"
      peername = File.open("#{taskPath}/migman/connections/ctrlconn/peername") {|f| f.readline}
      cmdline = File.open("/proc/#{filename}/cmdline") {|f| f.readline}
                                          
      nodeMap[peername.chop].addTask(Task.new(filename, cmdline))
  }
  
end

def parseNodeStatus(rootPath)
  nodeMap = {}
  Dir.foreach("#{rootPath}/nodes/") { |filename|
    fullPath = "#{rootPath}/nodes/#{filename}"
    # Iterate only through links (they represent IP of nodes)
    next if !File.symlink?(fullPath)
    linkContent = File.readlink(fullPath);
    nodeIndex = linkContent.split("/")[-1]
    state = File.open("#{fullPath}/state") {|f| f.readline}
    nodeMap[filename] = NodeInfo.new(nodeIndex, filename, state)
  }
  return nodeMap
end

def printNodeStatus(rootPath)
  nodeMap = parseNodeStatus(rootPath);
  parseTasksData(rootPath, nodeMap)  
  nodeMap.each_value { |node|
    node.printInfo();
  }
end

def printCoreNodeStatus()
  puts "== Core Node ==";
  printNodeStatus("/clondike/ccn/");
end

def printDetachedNodeStatus()
  puts "== Detached Node ==";
  printNodeStatus("/clondike/pen/");
end


puts "=================== Node status =============================";

printCoreNodeStatus();
printDetachedNodeStatus();
