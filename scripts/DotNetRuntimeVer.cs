public class DotNetRuntimeVer
{
  public string Name;
  public string Version;
  public string SP;
  public string Release;

  public DotNetRuntimeVer(string name, string version)
    : this(name, version, null, null)
  {
  }

  public DotNetRuntimeVer(string name, string version, string sp)
    : this(name, version, sp, null)
  {
  }

  public DotNetRuntimeVer(
    string name,
    string version,
    string sp,
    string release
  )
  {
    Name = name;
    // Version = version.IndexOf("v") != 0 ? "v" + version : version;
    Version = version;
    SP = sp;
    Release = release;
  }

  public override string ToString()
  {
    string s = "";

    if (Version.IndexOf(Name) > -1)
    {
      s += Version;
    }
    else
    {
      s += Name + " " + Version;
    }

    if (SP != null && !"".Equals(SP))
    {
      s += " SP " + SP;
    }
    if (Release != null && !"".Equals(Release))
    {
      s += " release " + Release;
    }

    return s;
  }
}
