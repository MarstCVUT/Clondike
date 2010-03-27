# Class representing a delegation of ACL
class Delegation
    # Back reference to ACL being delegated
    attr_reader :accessControlList
    # Entities authorized with this delegation rule
    attr_reader :authorizedEntities
    # If true, the delegations made by delegator will be persisted on the server
    # when it learns about them. As a result, the client needs to prove his
    # authorization only once and then the server keeps it persistent
    attr_accessor :persistDelegations
    # If true, delegator is responsible for propagating delegation updates to the server
    # after he issues them
    attr_accessor :eagerUpdate
    # Poll interval in seconds, how ofter the server asks delegators for their delegation updates
    # 0 means no polls are performed
    attr_accessor :pollInterval
    
    def initialize(accessControlList, persistDelegations, eagerUpdate, pollInterval)
        @accessControlList = accessControlList
        @persistDelegations = persistDelegations
        @eagerUpdate = eagerUpdate
        @pollInterval = pollInterval

        @authorizedEntities = Set.new
    end
    
    def addEntity(entity)
        @authorizedEntities.add(entity)
    end
    
    def removeEntity(entity)
        @authorizedEntities.delete(entity)
    end    
end