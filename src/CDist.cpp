#include "CDist.h"


void CDist::writeParamsToStream(ostream& out) const
{
  out << "numParams=" << getNumParams() << endl;
  for(int i=0; i<getNumParams()-1; i++)
    out << getParam(i) << " ";
  out << getParam(getNumParams()-1) << endl;
}
void CDist::readParamsFromStream(istream& in)
{
  string line;
  vector<string> tokens;
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="numParams")
    throw ndlexceptions::FileFormatError();
  int numParams=atol(tokens[1].c_str());
  CMatrix par(1, numParams);
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, " ");
  for(int i=0; i<numParams; i++)
    par.setVal(atof(tokens[i].c_str()), i);
  if(numParams==getNumParams())
    setParams(par);
  else
    throw ndlexceptions::FileFormatError();
}
bool CDist::equals(const CDist& dist, double tol) const
{
  if(getType()!=dist.getType())
    return false;
  if(getNumParams()!=dist.getNumParams())
    return false;
  CMatrix params(1, getNumParams());
  getParams(params);
  CMatrix distParams(1, getNumParams());
  dist.getParams(distParams);
  if(!params.equals(distParams, tol))
    return false;
  return true;
}

#ifdef _NDLMATLAB
mxArray* CDist::toMxArray() const
{
  int dims[1];
  dims[0] = 1;
  const char *fieldNames[] = {"type", "transforms"};
  mxArray* matlabArray = mxCreateStructArray(1, dims, 2, fieldNames);
    
  // type field.
  const char *typeName[1];
  string ty=getType();
  typeName[0] = ty.c_str();
  mxSetField(matlabArray, 0, "type", 
	     mxCreateCharMatrixFromStrings(1, typeName));
  
  // transforms field.
  mxSetField(matlabArray, 0, "transforms", transformsToMxArray());
  
  // Class specific code.
  addParamToMxArray(matlabArray);
  return matlabArray;

}
void CDist::fromMxArray(const mxArray* matlabArray) 
{
  string mxType = mxArrayExtractStringField(matlabArray, "type");
  if(mxType!=type)
    {
      throw ndlexceptions::MatlabInterfaceError("Error mismatch between saved type, " + mxType + ", and Class type, " + type + ".");
    }
  mxArray* transformArray = mxArrayExtractMxArrayField(matlabArray, "transforms");
  // transforms field.
  transformsFromMxArray(transformArray);
  extractParamFromMxArray(matlabArray);
}
void CDist::extractParamFromMxArray(const mxArray* matlabArray)
{
  nParams = mxArrayExtractIntField(matlabArray, "nParams");
  string pName;
  for(int i=0; i<nParams; i++)
    {
      pName=getParamName(i);
      setParam(mxArrayExtractDoubleField(matlabArray, pName), i);  
    }
}
void CDist::addParamToMxArray(mxArray* matlabArray) const
{
  mxAddField(matlabArray, "nParams");
  mxSetField(matlabArray, 0, "nParams", convertMxArray((double)nParams));
  string pName;
  for(int i=0; i<nParams; i++)
    {      
      pName = getParamName(i);
      mxAddField(matlabArray, pName.c_str());      
      mxSetField(matlabArray, 0, pName.c_str(), convertMxArray(getParam(i))); 
    }
}
#endif /* _NDLMATLAB*/
CGaussianDist::CGaussianDist()
{
  setInitParam();
}
CGaussianDist::CGaussianDist(const CGaussianDist& dist) : precision(dist.precision)
{
}
CGaussianDist::~CGaussianDist()
{
}
// Gaussian prior.
void CGaussianDist::setParam(double val, int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  switch(index)
    {
    case 0:
      precision = val;
      break;
    default:
      cerr << "No such parameter" << endl;
      exit(1);
    }

}
double CGaussianDist::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  switch(index)
    {
    case 0:
      return precision;
    default:
      cerr << "No such parameter" << endl;
      exit(1);
    }
  // WVB ADDED
  return -1;
}
void CGaussianDist::setInitParam()
{
  setType("gaussian");
  setNumParams(1);
  setName("Gaussian prior");
  setParamName("precision", 0);
  precision = 1.0;
  addTransform(new CNegLogLogitTransform, 0);
}
double CGaussianDist::logProb(double x) const
{
  return -0.5*precision*x*x -0.5*(ndlutil::LOGTWOPI - log(precision));
}
double CGaussianDist::getGradInput(double x) const
{
  return -precision*x;
}

// Gamma prior.
CGammaDist::CGammaDist()
{
  setInitParam();
}
CGammaDist::CGammaDist(const CGammaDist& dist) : a(dist.a), b(dist.b)
{
}
CGammaDist::~CGammaDist()
{
}
void CGammaDist::setParam( double val,  int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  switch(index)
    {
    case 0:
      a = val;
      break;
    case 1:
      b = val;
      break;
    default:
      cerr << "No such parameter" << endl;
      exit(1);
    }

}
double CGammaDist::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  switch(index)
    {
    case 0:
      return a;
    case 1:
      return b;
    default:
      cerr << "No such parameter" << endl;
      exit(1);
    }
  // WVB ADDED
  return -1;
}
void CGammaDist::setInitParam()
{
  setType("gamma");
  setName("gamma prior");
  setNumParams(2);  
  setParamName("a", 0);
  a=1e-6;
  setParamName("b", 1);
  b=1e-6;
  addTransform(new CNegLogLogitTransform, 0);
  addTransform(new CNegLogLogitTransform, 1);
}
double CGammaDist::logProb(double x) const
{
  return a*log(b) - ndlutil::gammaln(a) + ndlutil::xlogy(a-1.0,x)-b*x;  
}
double CGammaDist::getGradInput(double x) const
{
  return (a-1.0)/x - b;
}

// Wang's unusual prior from the GPDM thesis.
CWangDist::CWangDist()
{
  setInitParam();
}
CWangDist::CWangDist(const CWangDist& dist) : M(dist.M)
{
}
CWangDist::~CWangDist()
{
}
void CWangDist::setParam( double val,  int index)
{
  assert(index>=0);
  assert(index<getNumParams());
  switch(index)
    {
    case 0:
      M = val;
      break;
    default:
      cerr << "No such parameter" << endl;
      exit(1);
    }

}
double CWangDist::getParam(const int index) const
{
  assert(index>=0);
  assert(index<getNumParams());
  switch(index)
    {
    case 0:
      return M;
    default:
      cerr << "No such parameter" << endl;
      exit(1);
    }
  return -1;
}
void CWangDist::setInitParam()
{
  setType("wang");
  setName("Wang's GPDM prior");
  setNumParams(1);  
  setParamName("M", 0);
  M=1;
  addTransform(new CNegLogLogitTransform, 0);
}
double CWangDist::logProb(double x) const
{
  return -M*log(x);
}
double CWangDist::getGradInput(double x) const
{
  return -M/x;
}

#ifdef _NDLMATLAB
mxArray* CParamPriors::toMxArray() const
{
  int dims[1];
  // dists field.
  const char *transFieldNames[] = {"index", "type"};
  dims[0]=getNumDists();
  mxArray* distsArray = mxCreateStructArray(1, dims, 2, transFieldNames);
  CMatrix ind(1, 1);
  const char *compType[1];
  string trType;
  for(int i=0; i<getNumDists(); i++)
    {
      ind.setVals((double)(getDistIndex(i)+1));
      mxSetField(distsArray, i, "index", ind.toMxArray());
      trType = getDistType(i);
      compType[0] = trType.c_str();
      mxSetField(distsArray, i, "type", 
		 mxCreateCharMatrixFromStrings(1, compType));
    }
  return distsArray;
}
void CParamPriors::fromMxArray(const mxArray* distArray)
{
  int numDists = mxGetNumberOfElements(distArray);
  string distType;
  vector<int> distIndex;
  int counter = 0;
  for(int i=0; i<numDists; i++)
    {
      distType=mxArrayExtractStringField(distArray, "type", i);
      distIndex=mxArrayExtractVectorIntField(distArray, "index", i);
      for(int j=0; j<distIndex.size(); j++)
	{
	  counter++;
	  if(distType=="gamma")
	    addDist(new CGammaDist, distIndex[j]-1);
	  else if(distType=="wang")
	    addDist(new CWangDist, distIndex[j]-1);
	  else if(distType=="gaussian")
	    addDist(new CGaussianDist, distIndex[j]-1);
	  else
	    cerr << "Dist type " << distType << " is currently unknown."<< endl;
	}
    }    
}

#endif
void writeDistToStream(const CDist& dist, ostream& out)
{
  out << "distVersion=" << DISTVERSION << endl;
  out << "type=" << dist.getType() << endl;
  dist.writeParamsToStream(out);
}
CDist* readDistFromStream(istream& in)
{
  CDist* pdist;
  string line;
  vector<string> tokens;
  // first line is version info.
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="distVersion")
    throw ndlexceptions::FileFormatError();
  if(tokens[1]!="0.1")
    throw ndlexceptions::FileVersionError();
  // next line is type.
  tokens.clear();
  getline(in, line);
  ndlstrutil::tokenise(tokens, line, "=");
  if(tokens.size()>2 || tokens[0]!="type")
    throw ndlexceptions::FileFormatError();
  string type=tokens[1];
  if(type=="gaussian")
    pdist = new CGaussianDist();
  else if(type=="gamma")
    pdist = new CGammaDist();
  else if(type=="wang")
    pdist = new CWangDist();
  else
    throw ndlexceptions::FileFormatError();
  pdist->readParamsFromStream(in);
    
  return pdist;
}
