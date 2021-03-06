  // corner #1
  uc = 2.0*(texX+0.01)/CubeSize-1.0;
  vc = 2.0*(texY+0.01)/CubeSize-1.0;
  vc1 = vc;
  newCubeDir = SMCHECK_V3;
  dv = dot(Normal, newCubeDir);
  //dv += (1.0-abs(sign(dv)))*0.000001; // so it won't be zero
  t = ssd/dv;
  newCubeDir *= t;
  newCubeDist = dot(newCubeDir, newCubeDir);
  #ifdef SMCHECK_ASSIGN
  if (sldist >= newCubeDist) destvar = 1.0;
  #else
  if (sldist >= newCubeDist) return 1.0;
  #endif
  else
  {
    // corner #2
    //uc = 2.0*(texX+0.01)/CubeSize-1.0;
    vc = 2.0*(texY+0.99)/CubeSize-1.0;
    newCubeDir = SMCHECK_V3;
    dv = dot(Normal, newCubeDir);
    //dv += (1.0-abs(sign(dv)))*0.000001; // so it won't be zero
    t = ssd/dv;
    newCubeDir *= t;
    newCubeDist = dot(newCubeDir, newCubeDir);
    #ifdef SMCHECK_ASSIGN
    if (sldist >= newCubeDist) destvar = 1.0;
    #else
    if (sldist >= newCubeDist) return 1.0;
    #endif
    else
    {
      // corner #3
      uc = 2.0*(texX+0.99)/CubeSize-1.0;
      //vc = 2.0*(texY+0.99)/CubeSize-1.0;
      newCubeDir = SMCHECK_V3;
      dv = dot(Normal, newCubeDir);
      //dv += (1.0-abs(sign(dv)))*0.000001; // so it won't be zero
      t = ssd/dv;
      newCubeDir *= t;
      newCubeDist = dot(newCubeDir, newCubeDir);
      #ifdef SMCHECK_ASSIGN
      if (sldist >= newCubeDist) destvar = 1.0;
      #else
      if (sldist >= newCubeDist) return 1.0;
      #endif
      else
      {
        // corner #4
        //uc = 2.0*(texX+0.99)/CubeSize-1.0;
        //vc = 2.0*(texY+0.01)/CubeSize-1.0;
        vc = vc1;
        newCubeDir = SMCHECK_V3;
        dv = dot(Normal, newCubeDir);
        //dv += (1.0-abs(sign(dv)))*0.000001; // so it won't be zero
        t = ssd/dv;
        newCubeDir *= t;
        newCubeDist = dot(newCubeDir, newCubeDir);
        #ifdef SMCHECK_ASSIGN
        if (sldist >= newCubeDist) destvar = 1.0;
        #else
        if (sldist >= newCubeDist) return 1.0;
        return 0.0;
        #endif
      }
    }
  }
