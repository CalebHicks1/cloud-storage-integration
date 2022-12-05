using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace OneDrive;
public class OneDriveConfig
{
    public string[] Scopes { get; set; } = null!;
    public string ClientId { get; set; } = null!;
    public string TenantId { get; set; } = null!;
    public string RedirectUri { get; set; } = null!;
    public bool Debug { get; set; }
}
