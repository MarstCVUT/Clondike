require 'openssl'
require 'base64'

# Certificate representation
class Certificate
    attr_reader :id
    # Public key of issuer
    attr_reader :issuer
    # Unique identifier of this certificate. The number is unique only in scope
    # of single issuer
    attr_accessor :signature
    
    def initialize(id, issuer)
        @id = id
        @issuer = issuer
    end
    
    def sign(privateKey)
        @signature = privateKey.sign(OpenSSL::Digest::SHA1.new, toStringWithoutSignature)
    end    
    
    # Returns true, if the signature is valid, false otherwise
    def verifySignature
        return false if !@signature
        @issuer.verify( OpenSSL::Digest::SHA1.new, @signature, toStringWithoutSignature )
    end
    
    def save(fileName)
        File.open(fileName, File::CREAT|File::RDWR|File::TRUNC) { |file|
            file.puts "#{toStringWithSignature()}"
        }
    end
    
    def toStringWithoutSignature
        "Id: #{id}\nIssuer: #{issuer.undecorated_to_s}"
    end
    
    def toStringWithSignature
        @signature != nil ? "#{toStringWithoutSignature()}\nSignature: #{unsplitBase64(Base64.encode64(@signature))}" : toStringWithoutSignature()
    end
end

# Self-signed node certificate
class NodeCertificate<Certificate
    def initialize(id, issuer)
        super(id, issuer)
    end
end

# Certificate of a name (alias) for some remote node
class AliasCertificate<Certificate
    # Alias of the remote node 
    attr_reader :aliasName
    # Key of the node with the alias name
    attr_reader :aliasKey
    
    def initialize(id, issuer, aliasName, aliasKey)
        super(id, issuer)
        @aliasName = aliasName
        @aliasKey = aliasKey
    end
    
    def toStringWithoutSignature
        "#{super()}\nAlias name: #{aliasName}\nAlias key: #{aliasKey.undecorated_to_s}"
    end    
end

# Certificate of group membership
class GroupMembershipCertificate<Certificate
    # Local name of the group in context of issuer
    attr_reader :groupName
    # Public key of an entity being certified as a group member
    attr_reader :member
    
    def initialize(id, issuer, groupName, member)
        super(id, issuer)
        @groupName = groupName
        @member = member
    end
    
    def toStringWithoutSignature
        "#{super()}\nGroup name: #{groupName}\nMember: #{member.undecorated_to_s}"
    end
end

# Certificate, claiming the issuer is interested in changes in a particular group
# When this certificate is revoked, the node is not interested any more
class GroupInterestCertificate<Certificate
    # Local name of the group in context of issuer
    attr_reader :groupName
    # Public key of an entity who created the group
    attr_reader :master
    
    def initialize(id, issuer, groupName, master)
        super(id, issuer)
        @groupName = groupName
        @master = master
    end
    
    def toStringWithoutSignature
        "#{super()}\nGroup name: #{@groupName}\nMaster: #{@master.undecorated_to_s}"
    end
    
end

class AuthorizationCertificate<Certificate
    # Public key of entity being authorized
    attr_reader :authorizee
    # This is either a name of group in "authorizee" namespace that is being authorized
    # or nil, in case only a single entity is being authorized
    attr_reader :authorizedGroupName
    # This is the target node where the permission can be used.
    attr_reader :target
    # Name of the node groups (in "target" namespace) being authorized
    # nil, if only single "target" node is being authorized
    attr_reader :targetGroupName
    # Set of authorized permissions
    attr_reader :permissions
    # True, if the authorization carries as well delegation permission 
    # (i.e. it can be delegated furter by the authorizee)
    attr_reader :canDelegate
    # True, if the delegated rules shall be eager-back propagated to the issuer
    # Whenever this flag is true at some level at certificate chain, it MUST be true on all following chain steps
    attr_accessor :backPropagate
    
    def initialize(id, issuer, authorizee, authorizedGroupName, target, targetGroupName, permissions, canDelegate)
        super(id, issuer)
        @authorizee = authorizee
        @authorizedGroupName = authorizedGroupName      
        @target = target
        @targetGroupName = targetGroupName
        @permissions = permissions
        @canDelegate = canDelegate
        @backPropagate = false
        
        # This is now more a technical requirement, but we do not allow delegations of rights through group
        # Now, the delegations can be done only on per node basis
        raise "Authorizations for groups cannot allowe delegation!" if ( @authorizedGroupName != nil && canDelegate )
    end        
    
    def toStringWithoutSignature
        res = "#{super()}"
        res += "\nAuthorizee: #{authorizee.undecorated_to_s}"
        res += "\nAuthorized group: #{authorizedGroupName}" if authorizedGroupName        
        res += "\nTarget: #{target.undecorated_to_s}"
        res += "\nTarget group: #{targetGroupName}" if targetGroupName
        res += "\nCan delegate: #{canDelegate}"
        res += "\nBack propagate: #{backPropagate}"
        @permissions.each { |perm| 
            res += "\nPermission: #{perm.toCommandLine}"
        }        
        return res
    end    
end

class RevocationCertificate<Certificate
    # Public key of the node whose certificate is being revoked
    attr_reader :scope
    # Id of revoked certificate
    attr_reader :revokedId
    
    def initialize(id, issuer, scope, revokeId)
        super(id, issuer)
        @scope = scope
        @revokedId = revokeId
    end
    
    def toStringWithoutSignature
        "#{super()}\nRevoke id: #{revokedId}\nScope: #{scope.undecorated_to_s}"
    end    
    
end

def loadCertificate(fileName)
    valuePairs = {}
    permissions = []
    File.foreach(fileName) { |line|
       pair = line.match(/([^:]+):[ ]?(.+)/)
       if ( pair[1] != "Permission" ) then
          valuePairs[pair[1]] = pair[2]
       else
          permissions << pair[2]
       end
    }
    
    certificate = nil
    certificateId = valuePairs['Id']
    issuerKey = RSAPublicKey.undecoratedLoad(valuePairs['Issuer'])        

    if ( !permissions.empty? )
        # Authorization cert
        authorizee = RSAPublicKey.undecoratedLoad(valuePairs['Authorizee'])
        authorizedGroup = valuePairs['Authorized group']
        target = RSAPublicKey.undecoratedLoad(valuePairs['Target'])
        targetGroup = valuePairs['Target group']
        canDelegate = valuePairs['Can delegate'] == 'true'
        backPropagate = valuePairs['Back propagate'] == 'true'
        permissionSet = Set.new
        permissions.each { |perm|
            permissionSet.add(parsePermissionLine(perm))
        }        
        certificate = AuthorizationCertificate.new(certificateId, issuerKey, authorizee, authorizedGroup, target, targetGroup, permissionSet, canDelegate)
        certificate.backPropagate = backPropagate
    elsif ( valuePairs.has_key?("Member") )
        # Group cert
        groupName = valuePairs['Group name']
        member = RSAPublicKey.undecoratedLoad(valuePairs['Member'])
        certificate = GroupMembershipCertificate.new(certificateId, issuerKey, groupName, member)
    elsif ( valuePairs.has_key?("Master") )
        # Group interested certificate
        groupName = valuePairs['Group name']
        master = RSAPublicKey.undecoratedLoad(valuePairs['Master'])
        certificate = GroupInterestCertificate.new(certificateId, issuerKey, groupName, master)
    elsif ( valuePairs.has_key?("Alias name"))
        # Name alias cert
        aliasName = valuePairs['Alias name']
        aliasKey = RSAPublicKey.undecoratedLoad(valuePairs['Alias key'])
        certificate = AliasCertificate.new(certificateId, issuerKey, aliasName, aliasKey)
    elsif ( valuePairs.has_key?("Revoke id"))
        # Revocation certificate
        revokeId = valuePairs['Revoke id']
        scope = RSAPublicKey.undecoratedLoad(valuePairs['Scope'])
        certificate = RevocationCertificate.new(certificateId, issuerKey, scope, revokeId)
    else
        # Node certificate
        certificate = NodeCertificate.new(certificateId, issuerKey)
    end
    
    raise "Cannot parse certificate" if !certificate
    
    if ( valuePairs.has_key?('Signature'))
        signature = Base64.decode64(unsplitBase64(valuePairs['Signature']))
        certificate.signature = signature
    end
    
        
    return certificate
end
