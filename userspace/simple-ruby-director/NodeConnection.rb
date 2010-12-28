
class NodeConnection  
  # Index of slot where the connection was established
  attr_reader :slotIndex
  
  def initialize(slotIndex)
    @slotIndex = slotIndex;
  end
  
  def terminate()
    # TODO: Close the kernel socket slot
  end  
end

class OpenVPNNodeConnection<NodeConnection
  
  
  def terminate
    super.terminate
    #TODO: Close vpn
  end  
end