require 'trust/Entity.rb'
require 'trust/AccessControlList.rb'
 
# # In memory data structures related to trust management.
# This structure is build purely from issued certificates
class TrustInMemoryDataStore
    # Local identity
    attr_reader :identity
    # Hash of all known entities (name->Entity). Note: Name can be as well a composite "hash" name
    attr_reader :entities
    
    # Return a new instance of TrustInMemoryDataStore that is build based on certificates of provided identity
    def self.newDataStore(identity)
        dataStore = TrustInMemoryDataStore.new(identity)
        inMemoryCertificateChangeListener = InMemoryCertificateChangeListener.new(dataStore)
        identity.certificateStore.allValidCertificates.each { |certificate|
            inMemoryCertificateChangeListener.becomeValid(certificate)
        }
        identity.certificateStore.changeListener.addListener(inMemoryCertificateChangeListener)
        # TODO: Load certificates and add their mapppings
        return dataStore
    end
    
    # Adds new node alias mapping
    def addAliasMapping(aliasCertificate)
        #puts "Adding alias for #{aliasCertificate.aliasName}"
        raise "Conflicting alias name" if (entities.has_key?(aliasCertificate.aliasName) )

        remoteEntity = resolveRemoteNodeEntity(aliasCertificate.aliasKey)
        localEntity = LocalNodeEntity.new(aliasCertificate.aliasName)
        entities[aliasCertificate.aliasName] = localEntity
        localEntity.addEntity(remoteEntity, aliasCertificate)
    end
    
    # Removes alias mapping made by previous certificate
    def removeAliasMapping(aliasCertificate)
        # Resolves all dependend mappings
        entityToRemove = @entities[aliasCertificate.aliasName]
        entityToRemove.containedIn { |container|
            container.removeEntity(entityToRemove)
        }
        entityToRemove.entities { |ent|
            entityToRemove.removeEntity(ent)
        }
        
        # Remove only local mapping. The global mapping does not need to be modified
        @entities[aliasCertificate.aliasName] = nil
    end
    
    def addGroupMapping(groupCertificate)        
        groupEntity = resolveGroupEntity(groupCertificate.issuer, groupCertificate.groupName)        
        remoteEntity = resolveRemoteNodeEntity(groupCertificate.member)        
        #puts "ADDING GROUP MAPPING: #{groupEntity} .. issuer #{groupCertificate.issuer} .. remoteEntity #{remoteEntity}"
        groupEntity.addEntity(remoteEntity, groupCertificate)
    end
    
    def removeGroupMapping(groupCertificate)
        groupEntity = resolveGroupEntity(groupCertificate.issuer, groupCertificate.groupName)
        remoteEntity = resolveRemoteNodeEntity(groupCertificate.member)
        groupEntity.removeEntity(remoteEntity)
    end
    
    def addAuthorization(authorizationCertificate)
        # Here we assume the authorization certificates are only to remote entities, never to the aliases
        remoteEntity = authorizationCertificate.authorizedGroupName == nil ? resolveRemoteNodeEntity(authorizationCertificate.authorizee) : (resolveGroupEntity(authorizationCertificate.authorizee, authorizationCertificate.authorizedGroupName))
        targetEntity = authorizationCertificate.targetGroupName() == nil ? resolveRemoteNodeEntity(authorizationCertificate.target) : resolveRemoteGroupEntity(authorizationCertificate.target, authorizationCertificate.targetGroupName)
        acl = AccessControlList.new(targetEntity)
        authorizationCertificate.permissions.each { |perm| 
            acl.addPermission(perm)
        }
        
        isDelegation = authorizationCertificate.canDelegate()
        if ( isDelegation )
            delegation = acl.addNewDelegation()
            # TODO: Set delegation parameters from certificate
            remoteEntity.addDelegation(delegation, authorizationCertificate)
        else
            remoteEntity.addAuthorization(acl, authorizationCertificate)
        end
    end
    
    def removeAuthorization(authorizationCertificate)
        # Here we assume the authorization certificates are only to remote entities, never to the aliases
        remoteEntity = authorizationCertificate.authorizedGroupName == nil ? resolveRemoteNodeEntity(authorizationCertificate.authorizee) : (resolveGroupEntity(authorizationCertificate.authorizee, authorizationCertificate.authorizedGroupName))
        isDelegation = authorizationCertificate.canDelegate()
        if ( isDelegation )
           remoteEntity.delegations.each { |certifiedRelation|
               if ( certifiedRelation.certificate == authorizationCertificate )
                   remoteEntity.removeDelegation(certifiedRelation.object, authorizationCertificate)
                   return
               end
           } 
        else
           remoteEntity.authorizations.each { |certifiedRelation|
               if ( certifiedRelation.certificate == authorizationCertificate )
                   remoteEntity.removeAuthorization(certifiedRelation.object, authorizationCertificate)
                   return
               end
           } 
        end        
    end
    
    def registerGroupInterest(groupInterestCertificate)
        if ( groupInterestedCertificate.issuer == @identity.publicKey)
            registerRemoteGroupInterest(groupInterestCertificate)
        else
            registerLocalGroupInterest(groupInterestCertificate)
        end        
    end
    
    def deregisterGroupInterest(groupInterestCertificate)
        if ( groupInterestedCertificate.issuer == @identity.publicKey)
            deregisterRemoteGroupInterest(groupInterestCertificate)
        else
            deregisterLocalGroupInterest(groupInterestCertificate)
        end
    end
    
    def resolveGroupEntity(issuerKey, name)
        (issuerKey == @identity.publicKey || issuerKey == nil) ? resolveLocalGroupEntity(name) : resolveRemoteGroupEntity(issuerKey, name)
    end
    
private
    def initialize(localIdentity)
        @identity = localIdentity
        @entities = {}
    end
    
    def resolveRemoteNodeEntity(publicKey)
        #puts "RESOLVING REMOTE ENTITY: #{publicKey}"
        remoteEntity = @entities[publicKey]        
        if ( remoteEntity == nil ) then
            #puts "FAILED TO RESOLVE ENTITY WITH KEY: #{publicKey}.. entitied #{@entities.object_id}"
          remoteEntity = NodeEntity.new(publicKey)
          @entities[publicKey] = remoteEntity
        else
          raise "Invalid entity type" if !remoteEntity.kind_of?(NodeEntity)
        end
        remoteEntity
    end

    def resolveRemoteGroupEntity(issuerKey, name)
        key = [issuerKey, name]
        remoteEntity = entities[key]        
        if ( remoteEntity == nil ) then
          remoteEntity = RemoteGroupEntity.new(name, issuerKey)
          entities[key] = remoteEntity
        else
          raise "Invalid entity type" if !remoteEntity.kind_of?(RemoteGroupEntity)
        end
        remoteEntity
    end
    
    def resolveLocalNodeEntity(name)
        localEntity = entities[name]
        if ( localEntity == nil ) then
            localEntity = LocalNodeEntity.new(name)
            entities[name] = localEntity
        else
            raise "Invalid entity type" if !localEntity.kind_of?(LocalNodeEntity)
        end
        localEntity
    end
    
    def resolveLocalGroupEntity(name)
        localEntity = entities[name]
        if ( localEntity == nil ) then
            localEntity = LocalGroupEntity.new(name)
            entities[name] = localEntity
        else
            raise "Invalid entity type" if !localEntity.kind_of?(LocalGroupEntity)
        end
        localEntity
    end    

    def registerLocalGroupInterest(groupInterestCertificate)
        remoteEntity = resolveRemoteNodeEntity(groupInterestCertificate.issuer)
        localGroup = resolveLocalGroupEntity(groupInterestCertificate.groupName)        
        localGroup.registerInterestedEntity(remoteEntity)                
    end    
    
    def deregisterLocalGroupInterest(groupInterestCertificate)
        remoteEntity = resolveRemoteNodeEntity(groupInterestCertificate.issuer)
        localGroup = resolveLocalGroupEntity(groupInterestCertificate.groupName)        
        localGroup.deregisterInterestedEntity(remoteEntity)                
    end    
    
    def registerRemoteGroupInterest(groupInterestCertificate)
        #remoteEntity = resolveRemoteNodeEntity(groupInterestCertificate.issuer)
        remoteGroup = resolveRemoteGroupEntity(groupInterestCertificate.master, groupInterestCertificate.groupName)        
        remoteGroup.registerInterest(groupInterestCertificate)
    end        

    def deregisterRemoteGroupInterest(groupInterestCertificate)
        #remoteEntity = resolveRemoteNodeEntity(groupInterestCertificate.issuer)
        remoteGroup = resolveRemoteGroupEntity(groupInterestCertificate.master, groupInterestCertificate.groupName)        
        remoteGroup.deregisterInterest(groupInterestCertificate)
    end            
end

class InMemoryCertificateChangeListener
    def initialize(dataStore)
        @dataStore = dataStore
    end
    
    def becomeValid(certificate)
        case certificate
          when AliasCertificate
              @dataStore.addAliasMapping(certificate)
              return
          when GroupMembershipCertificate
              @dataStore.addGroupMapping(certificate)
              return
          when AuthorizationCertificate
              @dataStore.addAuthorization(certificate)
              return              
        end
    end
    
    def becomeInvalid(certificate)
        case certificate
          when AliasCertificate
              @dataStore.removeAliasMapping(certificate)
              return
          when GroupMembershipCertificate
              @dataStore.removeGroupMapping(certificate)
              return
          when AuthorizationCertificate
              @dataStore.removeAuthorization(certificate)
              return              
        end
    end    
end