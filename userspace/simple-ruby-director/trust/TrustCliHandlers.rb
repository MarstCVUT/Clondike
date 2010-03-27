class AliasHandler
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        nodeAlias = command.attributes['name'].value
        nodePublicKey = command.attributes['key'].value
        return "Alias already exists." if ( @trustManagement.dataStore.entities[nodeAlias] )
        
        @trustManagement.localIdentity.issueAliasCertificate(nodeAlias, nodePublicKey)
        return "Alias created."
    end
end

class DealiasHandler
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        nodeAlias = command.attributes['name'].value
        entity = @trustManagement.dataStore.entities[nodeAlias]
        return "Alias does not exists." if ( !entity )
        
        certifiedRelation = entity.entities().singleElement()[1]
        @trustManagement.localIdentity.revokeCertificate(certifiedRelation.certificate)
        return "Alias destroyed."
    end
end

class MemberHandler
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        groupName = command.attributes['group'].value
        nodePublicKey = command.attributes['key'].value
        group = @trustManagement.dataStore.entities[groupName]
        subentityRelation = group != nil ? group.subentity(nodePublicKey) : nil
        return "The node is already part of this group" if subentityRelation
        
        @trustManagement.localIdentity.issueGroupMembershipCertificate(groupName, nodePublicKey)
        return "Member assigned to group."
    end
end

class DememberHandler
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        groupName = command.attributes['group'].value
        nodePublicKey = command.attributes['key'].value
        group = @trustManagement.dataStore.entities[groupName]
        subentityRelation = group != nil ? group.subentity(nodePublicKey) : nil
        return "The node is not part of this group" if !subentityRelation
        
        @trustManagement.localIdentity.revokeCertificate(subentityRelation.certificate)
        return "Member removed from group."
    end
end

class AuthorizeHandlerBase
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        authorizee = resolveEntity(@trustManagement, command.attributes['authorizee'])
        return "Unknown authorizee" if (!authorizee)
        target = resolveEntity(@trustManagement, command.attributes['target'])
        return "Unknown target" if (!target)
        
        permissionAttribute = command.attributes['permission']
        permissionType = permissionAttribute.attributes["type"].value
        permissionOperation = permissionAttribute.attributes["operation"].value
        permissionObject = permissionAttribute.attributes["object"].value
        permission = parsePermissionLine("#{permissionType} #{permissionOperation} #{permissionObject}")
        canDelegate = permissionAttribute.attributes.include?("delegate")
        backPropagate = permissionAttribute.attributes.include?("backPropagate") || permissionAttribute.attributes["backPropagate"].value == 'true'
                
        return performOperation(authorizee[0], authorizee[1], target[0], target[1], permission, canDelegate, backPropagate)
    end
    
private

    def resolveEntity(trustManagement, attribute)        
        if ( attribute.attributes.include?("key"))
            key = attribute.attributes["key"].value
        else
            raise "Either key or name attribute is required" if !attribute.attributes.include?("name")
            name = attribute.attributes["name"]
            localEntity = trustManagement.dataStore.entities[name.value]
            return nil if !localEntity
            nodeEntity = localEntity.entities.singleElement[0]
            key = nodeEntity.name
        end

        if ( attribute.attributes.include?("group"))
            # Handle group entity
            groupName = attribute.attributes["group"]
            return [key, groupName]
        else
            # Handle node entity
            return [key, nil]
        end
    end
end

class AuthorizeHandler<AuthorizeHandlerBase
    def initialize(trustManagement)
        super(trustManagement)
    end
    
    def performOperation(authorizee, authorizeeGroup, target, targetGroup, permission, canDelegate, backPropagate)
      @trustManagement.localIdentity.issueAuthorizationCertificate(authorizee, authorizeeGroup, target, targetGroup, permission, canDelegate, backPropagate)
      return "Authorization issued."
    end
end

class DeauthorizeHandler<AuthorizeHandlerBase
    def initialize(trustManagement)
        super(trustManagement)
    end
    
    def performOperation(authorizee, authorizeeGroup, target, targetGroup, permission, canDelegate, backPropagate)
       authorizedEntity = authorizeeGroup == nil ? @trustManagement.dataStore.entities[authorizee] : @trustManagement.dataStore.resolveGroupEntity(authorizee, authorizeeGroup)
       return "Authorizee entity not found." if ( !authorizedEntity )
       
        authorizedEntity.authorizations.each { |certifiedRelation|
            authorization = certifiedRelation.object
            certificate = certifiedRelation.certificate
            
            result = handleAuthorizationRevocation(authorizee, authorizeeGroup, target, targetGroup, permission, certificate)
            next if !result
            return result
        }

        authorizedEntity.delegations.each { |certifiedRelation|
            authorization = certifiedRelation.object.accessControlList
            certificate = certifiedRelation.certificate
            
            result = handleAuthorizationRevocation(authorizee, authorizeeGroup, target, targetGroup, permission, certificate)
            next if !result
            return result
        }
        
        return "Authorizee does not have such a permission."
      #@trustManagement.localIdentity.issueAuthorizationCertificate(authorizee, authorizeeGroup, target, targetTargetGroup, permission, canDelegate)        
    end
private    
    def handleAuthorizationRevocation(authorizee, authorizeeGroup, target, targetGroup, permission, certificate)
        permissions = Set.new
        permissions.add(permission)
        
        # Only exactly matching authorizees & targets can be revoced
        return nil if certificate.authorizee != authorizee or certificate.authorizedGroupName != authorizeeGroup
        return nil if certificate.target != target or certificate.targetGroupName != targetGroup
        # Check bidirectional permissions equiality
        return nil if ( !areAllPermissionImplied(certificate.permissions, permissions))
        return nil if ( !areAllPermissionImplied(permissions, certificate.permissions))

        @trustManagement.localIdentity.revokeCertificate(certificate)
        return "Authorization removed."            
    end 
end

class ShowHandler
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        if command.attributes['node'] != nil            
            # Show single node
            showNodeAttribute = command.attributes['node'];
            if ( showNodeAttribute.attributes['name'] )
                result = printNodeFromRoot(showNodeAttribute.attributes['name'].value)
            else
                result = printNodeFromRoot(showNodeAttribute.attributes['key'].value)
            end
        elsif command.attributes['group'] != nil
            # Show single group
            showGroupAttribute = command.attributes['group'];
            scope = showGroupAttribute.attributes['key'] != nil ? showGroupAttribute.attributes['key'].value : nil
            name = showGroupAttribute.attributes['name'].value
            result = printGroup(scope, name)
        else
            # Show all
            result = ""
            @trustManagement.dataStore.entities.each_value { |entity|
                next if !isRootEntity(entity)
                result = result + printNode(entity, "")
                result = result + "------------------------------------------------------------------\n"
            }
        end
        
        return result
    end
    
private
    def printId(id)
        case id
          when RSAPublicKey
             return id.undecorated_to_s
          else
             return id                             
        end
    end   
    def printNodeInfo(entity, allowAliases = false)
        printFunction = allowAliases ? :printAliasedId : :printId
        case entity
          when LocalGroupEntity
              return "Group: #{send(printFunction,entity.name)}"
          when LocalNodeEntity
              return "Alias: #{send(printFunction,entity.name)}"
          when NodeEntity
              return "Node: #{send(printFunction,entity.name)}"
          when RemoteGroupEntity
              return "Remote group: #{send(printFunction,entity.scope)} - #{entity.name}"
          else
              return entity.class
        end       
    end
    
    def printNodeFromRoot(nodeId)
        entity = @trustManagement.dataStore.entities[nodeId]
        return "There is no entity with key #{nodeId}." if !entity                
        entity = findRootEntity(entity)

        printNode(entity, "")
    end
        
    def printNode(entity, prefix)
                
        result = "#{prefix}#{printNodeInfo(entity)}\n"
        
        entity.containedIn.each { |containingEntity|
            result = result + printNode(containingEntity, "  #{prefix}")
        }
        
        result = result + printAuthorizations(entity, prefix)
        result = result + printDelegations(entity, prefix)
        
        return result
    end
    
    def printGroup(scope, name)
        entity = @trustManagement.dataStore.resolveGroupEntity(scope, name)
        return "There is no entity with key #{nodeId}." if !entity                
        return "Entity #{nodeId} is not a group." if !entity.kind_of?(LocalGroupEntity) && !entity.kind_of?(RemoteGroupEntity)
        
        result = "#{printNodeInfo(entity)}\n"
        entity.entities.each_key { |subEntity|
            result = result + "  #{printNodeInfo(subEntity, true)}\n"
        }
        return result
    end
    
    # Prints alias instead of key (if there is some)
    def printAliasedId(id)
        entity = @trustManagement.dataStore.entities[id]
        return entity.name if ( entity.kind_of?(LocalNodeEntity))
        return "@@SELF@@" if (entity.name == @trustManagement.localIdentity.publicKey)
        # NOTE: If there are multiple aliases, random is choosen
        entity.containedIn.each { |containingEntity|
            return containingEntity.name if ( containingEntity.kind_of?(LocalNodeEntity))
        }
        
        return printId(entity.name)
    end
    
    def printAuthorizations(entity, prefix)
        return "" if ( entity.authorizations.empty? )
        
        result = "#{prefix}Authorizations:\n"
        entity.authorizations.each { |authorizationRelation|
            authorization = authorizationRelation.object
            authorization.permissions.each { |permission|
                result = result + "#{prefix}  #{permission.toCommandLine} (#{printNodeInfo(authorization.target)}\n"   
            }            
        }
        
        return result
    end
    
    def printDelegations(entity, prefix)
        return "" if ( entity.delegations.empty? )
        
        result = "#{prefix}Delegation:\n"
        entity.delegations.each { |delegationRelation|
            delegation = delegationRelation.object
            delegation.accessControlList.permissions.each { |permission|
                # TODO: Other delegation specific info
                result = result + "#{prefix}  #{permission.toCommandLine} (#{printNodeInfo(delegation.accessControlList.target)}\n"   
            }            
        }
        
        return result        
    end
    
    def isRootEntity(entity)            
        return true if ( !entity.kind_of?(CompositeEntity) )
        # We are either a group (= root element) or an entity that is not composed of anything
        return true if entity.entities.size != 1
          
        return false
    end
    
    def findRootEntity(entity)
        loop do
            return entity if isRootEntity(entity)
            entity = entity.entities.singleElement()[0]
        end        
    end
end

class CheckPermissionHandler
    def initialize(trustManagement)
        @trustManagement = trustManagement
    end
    
    def handle(command)
        entity = resolveEntity(@trustManagement, command.attributes['entity'])
        return "Unknown entity." if !entity
        
        permissionAttribute = command.attributes['permission']
        permissionType = permissionAttribute.attributes["type"].value
        permissionOperation = permissionAttribute.attributes["operation"].value
        permissionObject = permissionAttribute.attributes["object"].value        
        
        canAccess = @trustManagement.checkPermission(entity, permissionType, permissionOperation, permissionObject)
        return canAccess ? "Access granted." : "Access rejected."
    end
private
    def resolveEntity(trustManagement, attribute)        
        if ( attribute.attributes.include?("key"))
            key = attribute.attributes["key"].value
        else
            raise "Either key or name attribute is required" if !attribute.attributes.include?("name")
            name = attribute.attributes["name"]
            localEntity = trustManagement.dataStore.entities[name.value]
            return nil if ( !localEntity )
            nodeEntity = localEntity.entities.singleElement[0]
            key = nodeEntity.name
        end

        if ( attribute.attributes.include?("group"))
            # Handle group entity
            groupName = attribute.attributes["group"]
            return @trustManagement.dataStore.resolveGroupEntity(key, groupName)
        else
            # Handle node entity
            return @trustManagement.dataStore.entities[key]
        end
    end
end

# Helper method to register all know trust related cli internpreter handlers
def registerAllTrustHandler(trustManagement, interpreter)
    interpreter.addHandler("alias", AliasHandler.new(trustManagement))
    interpreter.addHandler("dealias", DealiasHandler.new(trustManagement))    
    interpreter.addHandler("member", MemberHandler.new(trustManagement))
    interpreter.addHandler("demember", DememberHandler.new(trustManagement))
    interpreter.addHandler("authorize", AuthorizeHandler.new(trustManagement))
    interpreter.addHandler("deauthorize", DeauthorizeHandler.new(trustManagement))    
    interpreter.addHandler("check", CheckPermissionHandler.new(trustManagement))    
    interpreter.addHandler("show", ShowHandler.new(trustManagement))
end