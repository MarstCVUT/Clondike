# Limiter of immigration requests based deciding based on provided array of limiters.. if any of the limiters denies immigrations, no request is accepted
# TODO: Extarct cound of currently immigrated tasks and check that value against limiters
class LimitersImmigrationController
  def initialize(limiters)
    @limiters = limiters
  end
  
  def onImmigrationRequest(node, execName)
    maximum = 100
    @limiters.each { |limiter|
      limiterMax = limiter.maximumAcceptCount()
      maximum = limiterMax if ( limiterMax < maximum )
    }
    
    # TODO: Not > 0, but > than current count of immigrated tasks
    return maximum > 0
  end
end