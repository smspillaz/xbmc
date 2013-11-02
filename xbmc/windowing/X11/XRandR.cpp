/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "XRandR.h"

#include <algorithm>
#include <iterator>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <string.h>
#include <sys/wait.h>
#include "system.h"
#include "PlatformInclude.h"
#include "utils/XBMCTinyXML.h"
#include "../xbmc/utils/log.h"

#if defined(TARGET_FREEBSD)
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "DynamicDll.h"
#include <X11/extensions/Xrandr.h>

class IDllXRandR
{
public:
  virtual ~IDllXRandR() {}

  virtual Bool XRRQueryExtension(Display *, int *, int *) = 0;
  virtual Status XRRQueryVersion(Display *, int *, int *) = 0;
  virtual XRRScreenConfiguration * XRRGetScreenInfo(Display *, Drawable) = 0;
  virtual void XRRFreeScreenConfigInfo(XRRScreenConfiguration *) = 0;
  virtual Status XRRSetScreenConfig(Display *, XRRScreenConfiguration *, Drawable, int, Rotation, Time) = 0;
  virtual Time XRRConfigTimes (XRRScreenConfiguration *, Time *) = 0;
  virtual XRRScreenSize * XRRConfigSizes (XRRScreenConfiguration *, int *) = 0;
  virtual short * XRRConfigRates (XRRScreenConfiguration *, int, int *) = 0;
  virtual SizeID XRRConfigCurrentConfiguration (XRRScreenConfiguration *, Rotation *) = 0;
  virtual short XRRConfigCurrentRate (XRRScreenConfiguration *) = 0;
  virtual int XRRSelectInput(Display *, Window, int) = 0;
  virtual XRRScreenResources * XRRGetScreenResources (Display *, Window) = 0;
  virtual void XRRFreeScreenResources (XRRScreenResources *) = 0;
  virtual XRROutputInfo * XRRGetOutputInfo (Display *, XRRScreenResources *, RROutput) = 0;
  virtual void XRRFreeOutputInfo (XRROutputInfo *) = 0;
  virtual XRRCrtcInfo * XRRGetCrtcInfo (Display *, XRRScreenResources *, RRCrtc) = 0;
  virtual Status XRRSetCrtcConfig (Display *, XRRScreenResources *, RRCrtc, Time, int, int, RRMode, Rotation, RROutput *, int) = 0;
  virtual void XRRFreeCrtcInfo (XRRCrtcInfo *) = 0;
};

class DllXRandR : public DllDynamic,
                  public IDllXRandR
{
public:

  DECLARE_DLL_WRAPPER(DllXRandR, DLL_PATH_XRANDR)

  DEFINE_METHOD3(Bool, XRRQueryExtension, (Display *p1, int *p2, int *p3));
  DEFINE_METHOD3(Status, XRRQueryVersion, (Display *p1, int *p2, int *p3));
  DEFINE_METHOD2(XRRScreenConfiguration *, XRRGetScreenInfo, (Display *p1, Drawable p2));
  DEFINE_METHOD1(void, XRRFreeScreenConfigInfo, (XRRScreenConfiguration *p1));
  DEFINE_METHOD6(Status, XRRSetScreenConfig, (Display *p1, XRRScreenConfiguration *p2, Drawable p3, int p4, Rotation p5, Time p6));
  DEFINE_METHOD2(Rotation, XRRConfigRotations, (XRRScreenConfiguration *p1, Rotation *p2));
  DEFINE_METHOD2(Time, XRRConfigTimes, (XRRScreenConfiguration *p1, Time *p2));
  DEFINE_METHOD2(XRRScreenSize *, XRRConfigSizes, (XRRScreenConfiguration *p1, int *p2));
  DEFINE_METHOD3(short *, XRRConfigRates, (XRRScreenConfiguration *p1, int p2, int *p3));
  DEFINE_METHOD2(SizeID, XRRConfigCurrentConfiguration, (XRRScreenConfiguration *p1, Rotation *p2));
  DEFINE_METHOD1(short, XRRConfigCurrentRate, (XRRScreenConfiguration *p1));
  DEFINE_METHOD3(int, XRRSelectInput, (Display *p1, Window p2, int p3));
  DEFINE_METHOD2(XRRScreenResources *, XRRGetScreenResources, (Display *p1, Window p2));
  DEFINE_METHOD1(void, XRRFreeScreenResources, (XRRScreenResources *p1));
  DEFINE_METHOD3(XRROutputInfo *, XRRGetOutputInfo, (Display *p1, XRRScreenResources *p2, RROutput p3));
  DEFINE_METHOD1(void, XRRFreeOutputInfo, (XRROutputInfo *p1));
  DEFINE_METHOD3(XRRCrtcInfo *, XRRGetCrtcInfo, (Display *p1, XRRScreenResources *p2, RRCrtc p3));
  DEFINE_METHOD10(Status, XRRSetCrtcConfig, (Display *p1, XRRScreenResources *p2, RRCrtc p3, Time p4, int p5, int p6, RRMode p7, Rotation p8, RROutput *p9, int p10));
  DEFINE_METHOD1(void, XRRFreeCrtcInfo, (XRRCrtcInfo *p1));

  BEGIN_METHOD_RESOLVE();
    RESOLVE_METHOD(XRRQueryExtension);
    RESOLVE_METHOD(XRRQueryVersion);
    RESOLVE_METHOD(XRRGetScreenInfo);
    RESOLVE_METHOD(XRRFreeScreenConfigInfo);
    RESOLVE_METHOD(XRRSetScreenConfig);
    RESOLVE_METHOD(XRRConfigRotations);
    RESOLVE_METHOD(XRRConfigTimes);
    RESOLVE_METHOD(XRRConfigSizes);
    RESOLVE_METHOD(XRRConfigRates);
    RESOLVE_METHOD(XRRConfigCurrentConfiguration);
    RESOLVE_METHOD(XRRConfigCurrentRate);
    RESOLVE_METHOD(XRRSelectInput);
    RESOLVE_METHOD(XRRGetScreenResources);
    RESOLVE_METHOD(XRRFreeScreenResources);
    RESOLVE_METHOD(XRRGetOutputInfo);
    RESOLVE_METHOD(XRRFreeOutputInfo);
    RESOLVE_METHOD(XRRGetCrtcInfo);
    RESOLVE_METHOD(XRRSetCrtcConfig);
    RESOLVE_METHOD(XRRFreeCrtcInfo);
  END_METHOD_RESOLVE();
};

namespace xbmc
{
namespace x11
{
namespace randr
{
class Mode :
  boost::noncopyable
{
public:

  typedef boost::shared_ptr<Mode> Ptr;

  Mode(XRRModeInfo *info);
  /* Mode is just observing XRRModeInfo, so there's no need
   * for explicit ownership */

  RRMode Identifier() const;
  const char * Name() const;
  unsigned int Width() const;
  unsigned int Height() const;
  unsigned int Rate() const;

private:

  XRRModeInfo *m_Info;
};

template<typename Resource> class ResourceGenerator;

template<typename Resource>
class ResourceIterator :
  public boost::iterator_facade<ResourceIterator<Resource>,
                                Resource,
                                boost::forward_traversal_tag>
{
public:

  typedef typename boost::remove_const<Resource>::type MutableResourceType;

  explicit ResourceIterator(const ResourceGenerator<Resource> &generator);
  ResourceIterator(const ResourceGenerator<Resource> &generator, size_t position);
  ResourceIterator(const ResourceIterator<Resource> &iterator);

private:

  friend class boost::iterator_core_access;

  bool equal(const ResourceIterator<Resource> &other) const;
  void increment();
  Resource & dereference() const;

  const ResourceGenerator<Resource> &m_Generator;
  size_t m_Position;
};

class ScreenResources;

template<typename Resource>
class ResourceGenerator
{
public:

  typedef Resource value_type;
  typedef const Resource const_value_type;

  virtual ~ResourceGenerator() {}

  typedef ResourceIterator<Resource> iterator;
  typedef ResourceIterator<const Resource> const_iterator;

  virtual iterator begin() = 0;
  virtual const_iterator begin() const = 0;

  virtual iterator end() = 0;
  virtual const_iterator end() const = 0;

  virtual const Resource & GetByPosition(size_t) const = 0;
  virtual Resource & GetByPosition(size_t) = 0;
};

template<typename Resource, typename Identifier>
class ResourceGeneratorByIdentifier : public ResourceGenerator<Resource>
{
public:

  typedef boost::function<Resource & (Identifier)> Fetch;
  typedef ResourceGenerator<Resource> Base;

  ResourceGeneratorByIdentifier(Identifier *ids,
                                int nids,
                                const Fetch &fetch);

  typename Base::iterator begin();
  typename Base::const_iterator begin() const;

  typename Base::iterator end();
  typename Base::const_iterator end() const;

private:

  /* May throw std::out_of_range */
  const Resource & GetByPosition(size_t) const;
  Resource & GetByPosition(size_t);

  Identifier *m_Identifiers;
  size_t m_NIdentifiers;
  Fetch m_Fetch;
};

class OutputInfo :
  boost::noncopyable
{
public:

  typedef boost::shared_ptr<OutputInfo> Ptr;

  OutputInfo(RROutput identifier,
             XRROutputInfo *info,
             const ScreenResources &resources,
             const IDllXRandR &randrLibrary);
  ~OutputInfo();

  RROutput Identifier() const;
  const char * Name() const;
  ResourceGeneratorByIdentifier<const Mode, RRMode> Modes() const;
  const Mode & PreferredMode() const;
  unsigned long HeightInMillimeters() const;
  unsigned long WidthInMillimeters() const;

private:

  RROutput m_Identifier;
  XRROutputInfo *m_Info;
  const ScreenResources &m_Resources;
  IDllXRandR &m_randrLibrary;
};

class CRTController :
  boost::noncopyable
{
public:

  typedef boost::shared_ptr<CRTController> Ptr;

  CRTController(RRCrtc identifier,
                XRRCrtcInfo *info,
                const ScreenResources &resources,
                const IDllXRandR &randrLibrary);
  ~CRTController();

  RRCrtc Identifier() const;

  /* ConnectedOutputs are outputs that are being actively displayed to
   * by this CRTController. For instance, there might be multiple
   * outputs displaying the image on this CRTController */
  ResourceGeneratorByIdentifier<const OutputInfo, RROutput> ConnectedOutputs() const;

  /* PossibleOutputs are all the outputs that the CRTController could
   * possibly be connected to and display an image to. They would all
   * be displaying the same image at the same mode */
  ResourceGeneratorByIdentifier<const OutputInfo, RROutput> PossibleOutputs() const;

private:

  RRCrtc m_Identifier;
  XRRCrtcInfo *m_Info;
  const ScreenResources &m_Resources;
  IDllXRandR &m_randrLibrary;
};

class RequestBuilder
{
public:

  RequestBuilder(Display *dpy, XRRScreenResources *, RRCrtc, IDllXRandR &);

  RequestBuilder & SetMode(RRMode mode);
  RequestBuilder & SetRotation(Rotation rotation);
  RequestBuilder & SetOffset(int x, int y);
  RequestBuilder & ConnectOutput(RROutput output);

  void operator() ();

private:

  Display *m_Dpy;
  XRRScreenResources *m_Resources;
  RRCrtc m_CRTController;
  IDllXRandR &m_xrandrLibrary;

  RRMode m_Mode;
  Rotation m_Rotation;
  int m_XOffset;
  int m_YOffset;
  std::vector<RROutput> m_Outputs;
};

class ScreenResources
{
public:

  ScreenResources(Display *dpy, XRRScreenResources *, IDllXRandR &);
  ~ScreenResources();

  /* We only want to look up modes and outputs
   * by their ID */
  const Mode & FetchModeByID(RRMode) const;
  const OutputInfo & FetchOutputInfoByID(RROutput) const;
  const CRTController & FetchCRTControllerByID(RRCrtc) const;

  /* CRTControllers's are direct children of this node
   * we we want to provide a range of them */
  ResourceGeneratorByIdentifier<const CRTController, RRCrtc> CRTControllers() const;

  /* The main function to change a crtc's configuration. A CRTController
   * is what the GPU actually renders some framebuffer image on to
   * in a format that monitors can understand. Any arbitrary number
   * of monitors can be connected to a CRTController, but they will
   * all be displaying the same image and at the same mode.
   *
   * This method returns a RequestBuilder which has methods
   * that allow chained calls to modify its state before it is
   * called as a function */
  RequestBuilder ChangeCRTControllerConfiguration (RRCrtc);

  /* Something happened on the server side which means our
   * internal cache is no longer good. Clear all the vectors
   * and reload our XRRScreenResources */
  void Invalidate();

private:

  Display *m_Dpy;
  XRRScreenResources *m_Resources;
  IDllXRandR &m_xrandrLibrary;

  /* Mode, OutputInfo and CRTController are just objects that are owned by
   * ScreenResources - these vectors purely serve as an
   * ordered cache so that we can quickly do a lookup of an
   * existing mode or create it from a const method.
   *
   * As soon as the screen configuration changes we need
   * to process the event on the libXRandR side and invalidate
   * these caches, as the available modes or outputs might
   * now be different.
   *
   * These can't be value types because they take ownership of
   * internal XRandR structures, so they need to be dynamically
   * allocated object types considering the lack of move
   * semantics in C++98.
   */
  mutable std::vector<Mode::Ptr> m_Modes;
  mutable std::vector<OutputInfo::Ptr> m_Outputs;
  mutable std::vector<CRTController::Ptr> m_CRTControllers;
};
}
}
}

namespace xxr = xbmc::x11::randr;

xxr::Mode::Mode(XRRModeInfo *info) :
  m_Info(info)
{
}

RRMode xxr::Mode::Identifier() const
{
  return m_Info->id;
}

const char * xxr::Mode::Name() const
{
  return m_Info->name;
}

unsigned int xxr::Mode::Width() const
{
  return m_Info->width;
}

unsigned int xxr::Mode::Height() const
{
  return m_Info->height;
}

unsigned int xxr::Mode::Rate() const
{
  return m_Info->vTotal; /* XXX */
}

template<typename Resource>
xxr::ResourceIterator<Resource>::ResourceIterator(const ResourceIterator<Resource> &iterator) :
  m_Generator(iterator.m_Generator),
  m_Position(iterator.m_Position)
{
}

template<typename Resource>
xxr::ResourceIterator<Resource>::ResourceIterator(const ResourceGenerator<Resource> &generator,
                                                  size_t position) :
  m_Generator(generator),
  m_Position(position)
{
}

template<typename Resource>
xxr::ResourceIterator<Resource>::ResourceIterator(const ResourceGenerator<Resource> &generator) :
  m_Generator(generator),
  m_Position(0)
{
}

template<typename Resource>
bool xxr::ResourceIterator<Resource>::equal(const ResourceIterator<Resource> &other) const
{
  return m_Position == other.m_Position;
}

template<typename Resource>
void xxr::ResourceIterator<Resource>::increment()
{
  ++m_Position;
}

template<typename Resource>
Resource & xxr::ResourceIterator<Resource>::dereference() const
{
  return m_Generator.GetByPosition(m_Position);
}

template <typename Resource, typename Identifier>
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::ResourceGeneratorByIdentifier(Identifier *ids,
                                                                                        int nids,
                                                                                        const Fetch &fetch) :
  m_Identifiers(ids),
  m_NIdentifiers(nids),
  m_Fetch(fetch)
{
}

template<typename Resource, typename Identifier>
typename xxr::ResourceGenerator<Resource>::iterator
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::begin()
{
  typedef xxr::ResourceGenerator<Resource> Base;
  return typename Base::iterator(*this);
}

template<typename Resource, typename Identifier>
typename xxr::ResourceGenerator<Resource>::const_iterator
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::begin() const
{
  typedef xxr::ResourceGenerator<Resource> Base;
  return typename Base::const_iterator(*this);
}

template<typename Resource, typename Identifier>
typename xxr::ResourceGenerator<Resource>::iterator
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::end()
{
  typedef xxr::ResourceGenerator<Resource> Base;
  return typename Base::iterator(*this, m_NIdentifiers);
}

template<typename Resource, typename Identifier>
typename xxr::ResourceGenerator<Resource>::const_iterator
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::end() const
{
  typedef xxr::ResourceGenerator<Resource> Base;
  return typename Base::const_iterator(*this, m_NIdentifiers);
}

template<typename Resource, typename Identifier>
const Resource &
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::GetByPosition(size_t pos) const
{
  if (pos >= m_NIdentifiers)
  {
    std::stringstream ss;
    ss << "position "
       << pos
       << " is out of range of available identifiers "
       << m_NIdentifiers;
    throw std::out_of_range(ss.str());
  }

  return m_Fetch(m_Identifiers[pos]);
}

template<typename Resource, typename Identifier>
Resource &
xxr::ResourceGeneratorByIdentifier<Resource, Identifier>::GetByPosition(size_t pos)
{
  typedef xxr::ResourceGeneratorByIdentifier<Resource, Identifier> T;
  const T &this_const(const_cast<const T &>(*this));
  return const_cast<Resource &>(this_const.GetByPosition(pos));
}

xxr::OutputInfo::OutputInfo(RROutput identifier,
                            XRROutputInfo *info,
                            const ScreenResources &resources,
                            const IDllXRandR &randrLibrary) :
  m_Identifier(identifier),
  m_Info(info),
  m_Resources(resources),
  m_randrLibrary(const_cast<IDllXRandR &>(randrLibrary))
{
}

xxr::OutputInfo::~OutputInfo()
{
  m_randrLibrary.XRRFreeOutputInfo(m_Info); /* XXX */
}

RROutput xxr::OutputInfo::Identifier() const
{
  return m_Identifier;
}

const char * xxr::OutputInfo::Name() const
{
  return m_Info->name;
}

xxr::ResourceGeneratorByIdentifier<const xxr::Mode, RRMode> xxr::OutputInfo::Modes() const
{
  typedef xxr::ResourceGeneratorByIdentifier<const xxr::Mode, RRMode> ModeGenerator;
  ModeGenerator::Fetch fetch(boost::bind(&ScreenResources::FetchModeByID,
                                         &m_Resources,
                                         _1));
  return xxr::ResourceGeneratorByIdentifier<const xxr::Mode, RRMode>(m_Info->modes,
                                                                     m_Info->nmode,
                                                                     fetch);
}

namespace
{
const xxr::Mode & FindLargestModeInRange(xxr::ResourceIterator<const xxr::Mode> begin,
                                         xxr::ResourceIterator<const xxr::Mode> end)
{
  const xxr::Mode *mode = &(*begin);
  unsigned int area = mode->Width() * mode->Height();

  for(xxr::ResourceIterator<const xxr::Mode> it = ++begin;
      it != end;
      ++it)
  {
    const xxr::Mode &candidateMode(*it);
    unsigned int modeArea = candidateMode.Width() * candidateMode.Height();

    if (modeArea > area)
    {
      area = modeArea;
      mode = &candidateMode;
    }
  }

  return *mode;
}
}

const xxr::Mode & xxr::OutputInfo::PreferredMode() const
{
  /* XXX: In XRandR, an output can have many preferred
   * modes, but we only care about the most preferred
   * mode. If an output has no preferred mode or many
   * preferred modes then search for the largest of
   * those */
  if (m_Info->npreferred == 1)
    return m_Resources.FetchModeByID(m_Info->modes[0]);

  typedef xxr::ResourceGeneratorByIdentifier<const xxr::Mode, RRMode> ModeGenerator;
  if (!m_Info->npreferred)
  {
    const ModeGenerator generator(Modes());
    return FindLargestModeInRange(generator.begin(),
                                  generator.end());
  }
  else
  {
    ModeGenerator::Fetch fetch(boost::bind(&ScreenResources::FetchModeByID,
                                           &m_Resources,
                                           _1));

    /* XRROutputInfo::modes always starts with the preferred
     * modes */
    const ModeGenerator generator(m_Info->modes,
                                  m_Info->npreferred,
                                  fetch);

    return FindLargestModeInRange(generator.begin(),
                                  generator.end());
  }
}

unsigned long xxr::OutputInfo::HeightInMillimeters() const
{
  return m_Info->mm_height;
}

unsigned long xxr::OutputInfo::WidthInMillimeters() const
{
  return m_Info->mm_width;
}

xxr::CRTController::CRTController(RRCrtc identifier,
                                  XRRCrtcInfo *info,
                                  const ScreenResources &resources,
                                  const IDllXRandR &randrLibrary) :
  m_Identifier(identifier),
  m_Info(info),
  m_Resources(resources),
  m_randrLibrary(const_cast<IDllXRandR &>(randrLibrary))
{
}

xxr::CRTController::~CRTController()
{
  m_randrLibrary.XRRFreeCrtcInfo(m_Info); /* XXX */
}

RRCrtc xxr::CRTController::Identifier() const
{
  return m_Identifier;
}

xxr::ResourceGeneratorByIdentifier<const xxr::OutputInfo, RROutput>
xxr::CRTController::PossibleOutputs() const
{
  typedef xxr::ResourceGeneratorByIdentifier<const OutputInfo, RROutput> OutputGenerator;
  OutputGenerator::Fetch fetch(boost::bind(&ScreenResources::FetchOutputInfoByID,
                                           &m_Resources,
                                           _1));

  return OutputGenerator(m_Info->possible,
                         m_Info->npossible,
                         fetch);
}

xxr::ResourceGeneratorByIdentifier<const xxr::OutputInfo, RROutput>
xxr::CRTController::ConnectedOutputs() const
{
  typedef xxr::ResourceGeneratorByIdentifier<const OutputInfo, RROutput> OutputGenerator;
  OutputGenerator::Fetch fetch(boost::bind(&ScreenResources::FetchOutputInfoByID,
                                           &m_Resources,
                                           _1));

  return OutputGenerator(m_Info->outputs,
                         m_Info->noutput,
                         fetch);
}

xxr::RequestBuilder::RequestBuilder(Display *dpy,
                                    XRRScreenResources *resources,
                                    RRCrtc crtc,
                                    IDllXRandR &randrLibrary) :
  m_Dpy(dpy),
  m_Resources(resources),
  m_CRTController(crtc),
  m_xrandrLibrary(randrLibrary),
  m_Mode(0),
  m_Rotation(RR_Rotate_0),
  m_XOffset(0),
  m_YOffset(0)
{
}

xxr::RequestBuilder & xxr::RequestBuilder::SetMode(RRMode mode)
{
  m_Mode = mode;
  return *this;
}

xxr::RequestBuilder & xxr::RequestBuilder::SetRotation(Rotation rotation)
{
  m_Rotation = rotation;
  return *this;
}

xxr::RequestBuilder & xxr::RequestBuilder::SetOffset(int x, int y)
{
  m_XOffset = x;
  m_YOffset = y;
  return *this;
}

xxr::RequestBuilder & xxr::RequestBuilder::ConnectOutput(RROutput output)
{
  if (std::find(m_Outputs.begin(),
                m_Outputs.end(),
                output) != m_Outputs.end())
    return *this;

  m_Outputs.push_back(output);
  return *this;
}

void xxr::RequestBuilder::operator()()
{
  m_xrandrLibrary.XRRSetCrtcConfig(m_Dpy,
                                   m_Resources,
                                   m_CRTController,
                                   CurrentTime,
                                   m_XOffset,
                                   m_YOffset,
                                   m_Mode,
                                   m_Rotation,
                                   &m_Outputs[0],
                                   m_Outputs.size());
}

xxr::ScreenResources::ScreenResources(Display *dpy,
                                      XRRScreenResources *resources,
                                      IDllXRandR &randrLibrary) :
  m_Dpy(dpy),
  m_Resources(resources),
  m_xrandrLibrary(randrLibrary)
{
}

xxr::ScreenResources::~ScreenResources()
{
  m_xrandrLibrary.XRRFreeScreenResources(m_Resources);
}

namespace
{
template<typename Resource>
bool IDIsLowerThanProvidedID(const Resource &resource,
                             XID id)
{
  return resource->Identifier() < id;
}
}

const xxr::Mode & xxr::ScreenResources::FetchModeByID(RRMode id) const
{
  /* First, search for this mode in the available cached modes */
  std::vector<xxr::Mode::Ptr>::iterator it(std::lower_bound(m_Modes.begin(),
                                                            m_Modes.end(),
                                                            id,
                                                            IDIsLowerThanProvidedID<xxr::Mode::Ptr>));

  if (it != m_Modes.end() &&
      (*it)->Identifier() == id)
    return *(*it);

  for (int i = 0; i < m_Resources->nmode; ++i)
  {
    if (m_Resources->modes[i].id == id)
    {
      /* Insert into cache */
      it = m_Modes.insert(it,
                          boost::make_shared<xxr::Mode>(&m_Resources->modes[i]));
      return *(*it);
    }
  }

  std::stringstream ss;
  ss << "No mode with id " << id << " exists on this screen";

  throw std::logic_error(ss.str());
}

const xxr::OutputInfo & xxr::ScreenResources::FetchOutputInfoByID(RROutput id) const
{
  /* First, search for this output in the available cached outputs */
  std::vector<xxr::OutputInfo::Ptr>::iterator it(std::lower_bound(m_Outputs.begin(),
                                                                  m_Outputs.end(),
                                                                  id,
                                                                  IDIsLowerThanProvidedID<xxr::OutputInfo::Ptr>));

  if (it != m_Outputs.end() &&
      (*it)->Identifier() == id)
    return *(*it);

  XRROutputInfo *info = m_xrandrLibrary.XRRGetOutputInfo(m_Dpy,
                                                         m_Resources,
                                                         id);

  if (!info)
  {
    std::stringstream ss;
    ss << "No output with id " << id << " exists on this screen";
    throw std::logic_error(ss.str());
  }

  /* Insert into cache */
  it = m_Outputs.insert(it,
                        boost::make_shared<xxr::OutputInfo>(id,
                                                            info,
                                                            *this,
                                                            m_xrandrLibrary));
  return *(*it);
}

const xxr::CRTController & xxr::ScreenResources::FetchCRTControllerByID(RRCrtc id) const
{
  /* First, search for this crtc in the available cached crtcs */
  std::vector<xxr::CRTController::Ptr>::iterator it(std::lower_bound(m_CRTControllers.begin(),
                                                                     m_CRTControllers.end(),
                                                                     id,
                                                                     IDIsLowerThanProvidedID<xxr::CRTController::Ptr>));

  if (it != m_CRTControllers.end() &&
      (*it)->Identifier() == id)
    return *(*it);

  XRRCrtcInfo *info = m_xrandrLibrary.XRRGetCrtcInfo(m_Dpy,
                                                     m_Resources,
                                                     id);

  if (!info)
  {
    std::stringstream ss;
    ss << "No crtc with id " << id << " exists on this screen";
    throw std::logic_error(ss.str());
  }

  /* Insert into cache */
  it = m_CRTControllers.insert(it,
                               boost::make_shared<xxr::CRTController> (id,
                                                                       info,
                                                                       *this,
                                                                       m_xrandrLibrary));
  return *(*it);
}

xxr::ResourceGeneratorByIdentifier<const xxr::CRTController, RRCrtc>
xxr::ScreenResources::CRTControllers() const
{
  typedef xxr::ResourceGeneratorByIdentifier<const CRTController, RRCrtc> CRTGenerator;
  CRTGenerator::Fetch fetch (boost::bind(&ScreenResources::FetchCRTControllerByID,
                                         this,
                                         _1));

  return CRTGenerator(m_Resources->crtcs,
                      m_Resources->ncrtc,
                      fetch);
}

xxr::RequestBuilder xxr::ScreenResources::ChangeCRTControllerConfiguration(RRCrtc crtc)
{
  return xxr::RequestBuilder(m_Dpy,
                             m_Resources,
                             crtc,
                             m_xrandrLibrary);
}

void xxr::ScreenResources::Invalidate()
{
  m_Modes.clear();
  m_Outputs.clear();
  m_CRTControllers.clear();
}

using namespace std;

CXRandR::CXRandR(bool query)
{
  m_bInit = false;
  if (query)
    Query();
}

bool CXRandR::Query(bool force)
{
  if (!force)
    if (m_bInit)
      return m_outputs.size() > 0;

  m_bInit = true;

  if (getenv("XBMC_BIN_HOME") == NULL)
    return false;

  m_outputs.clear();
  m_current.clear();

  CStdString cmd;
  cmd  = getenv("XBMC_BIN_HOME");
  cmd += "/xbmc-xrandr";

  FILE* file = popen(cmd.c_str(),"r");
  if (!file)
  {
    CLog::Log(LOGERROR, "CXRandR::Query - unable to execute xrandr tool");
    return false;
  }


  CXBMCTinyXML xmlDoc;
  if (!xmlDoc.LoadFile(file, TIXML_DEFAULT_ENCODING))
  {
    CLog::Log(LOGERROR, "CXRandR::Query - unable to open xrandr xml");
    pclose(file);
    return false;
  }
  pclose(file);

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcasecmp(pRootElement->Value(), "screen") != 0)
  {
    // TODO ERROR
    return false;
  }

  for (TiXmlElement* output = pRootElement->FirstChildElement("output"); output; output = output->NextSiblingElement("output"))
  {
    XOutput xoutput;
    xoutput.name = output->Attribute("name");
    xoutput.name.TrimLeft(" \n\r\t");
    xoutput.name.TrimRight(" \n\r\t");
    xoutput.isConnected = (strcasecmp(output->Attribute("connected"), "true") == 0);
    xoutput.w = (output->Attribute("w") != NULL ? atoi(output->Attribute("w")) : 0);
    xoutput.h = (output->Attribute("h") != NULL ? atoi(output->Attribute("h")) : 0);
    xoutput.x = (output->Attribute("x") != NULL ? atoi(output->Attribute("x")) : 0);
    xoutput.y = (output->Attribute("y") != NULL ? atoi(output->Attribute("y")) : 0);
    xoutput.wmm = (output->Attribute("wmm") != NULL ? atoi(output->Attribute("wmm")) : 0);
    xoutput.hmm = (output->Attribute("hmm") != NULL ? atoi(output->Attribute("hmm")) : 0);

    if (!xoutput.isConnected)
       continue;

    bool hascurrent = false;
    for (TiXmlElement* mode = output->FirstChildElement("mode"); mode; mode = mode->NextSiblingElement("mode"))
    {
      XMode xmode;
      xmode.id = mode->Attribute("id");
      xmode.name = mode->Attribute("name");
      xmode.hz = atof(mode->Attribute("hz"));
      xmode.w = atoi(mode->Attribute("w"));
      xmode.h = atoi(mode->Attribute("h"));
      xmode.isPreferred = (strcasecmp(mode->Attribute("preferred"), "true") == 0);
      xmode.isCurrent = (strcasecmp(mode->Attribute("current"), "true") == 0);
      xoutput.modes.push_back(xmode);
      if (xmode.isCurrent)
      {
        m_current.push_back(xoutput);
        hascurrent = true;
      }
    }
    if (hascurrent)
      m_outputs.push_back(xoutput);
    else
      CLog::Log(LOGWARNING, "CXRandR::Query - output %s has no current mode, assuming disconnected", xoutput.name.c_str());
  }
  return m_outputs.size() > 0;
}

std::vector<XOutput> CXRandR::GetModes(void)
{
  Query();
  return m_outputs;
}

void CXRandR::SaveState()
{
  Query(true);
}

void CXRandR::RestoreState()
{
  vector<XOutput>::iterator outiter;
  for (outiter=m_current.begin() ; outiter!=m_current.end() ; outiter++)
  {
    vector<XMode> modes = (*outiter).modes;
    vector<XMode>::iterator modeiter;
    for (modeiter=modes.begin() ; modeiter!=modes.end() ; modeiter++)
    {
      XMode mode = *modeiter;
      if (mode.isCurrent)
      {
        SetMode(*outiter, mode);
        return;
      }
    }
  }
}

bool CXRandR::SetMode(XOutput output, XMode mode)
{
  if ((output.name == m_currentOutput && mode.id == m_currentMode) || (output.name == "" && mode.id == ""))
    return true;

  Query();

  // Make sure the output exists, if not -- complain and exit
  bool isOutputFound = false;
  XOutput outputFound;
  for (size_t i = 0; i < m_outputs.size(); i++)
  {
    if (m_outputs[i].name == output.name)
    {
      isOutputFound = true;
      outputFound = m_outputs[i];
    }
  }

  if (!isOutputFound)
  {
    CLog::Log(LOGERROR, "CXRandR::SetMode: asked to change resolution for non existing output: %s mode: %s", output.name.c_str(), mode.id.c_str());
    return false;
  }

  // try to find the same exact mode (same id, resolution, hz)
  bool isModeFound = false;
  XMode modeFound;
  for (size_t i = 0; i < outputFound.modes.size(); i++)
  {
    if (outputFound.modes[i].id == mode.id)
    {
      if (outputFound.modes[i].w == mode.w &&
          outputFound.modes[i].h == mode.h &&
          outputFound.modes[i].hz == mode.hz)
      {
        isModeFound = true;
        modeFound = outputFound.modes[i];
      }
      else
      {
        CLog::Log(LOGERROR, "CXRandR::SetMode: asked to change resolution for mode that exists but with different w/h/hz: %s mode: %s. Searching for similar modes...", output.name.c_str(), mode.id.c_str());
        break;
      }
    }
  }

  if (!isModeFound)
  {
    for (size_t i = 0; i < outputFound.modes.size(); i++)
    {
      if (outputFound.modes[i].w == mode.w &&
          outputFound.modes[i].h == mode.h &&
          outputFound.modes[i].hz == mode.hz)
      {
        isModeFound = true;
        modeFound = outputFound.modes[i];
        CLog::Log(LOGWARNING, "CXRandR::SetMode: found alternative mode (same hz): %s mode: %s.", output.name.c_str(), outputFound.modes[i].id.c_str());
      }
    }
  }

  if (!isModeFound)
  {
    for (size_t i = 0; i < outputFound.modes.size(); i++)
    {
      if (outputFound.modes[i].w == mode.w &&
          outputFound.modes[i].h == mode.h)
      {
        isModeFound = true;
        modeFound = outputFound.modes[i];
        CLog::Log(LOGWARNING, "CXRandR::SetMode: found alternative mode (different hz): %s mode: %s.", output.name.c_str(), outputFound.modes[i].id.c_str());
      }
    }
  }

  // Let's try finding a mode that is the same
  if (!isModeFound)
  {
    CLog::Log(LOGERROR, "CXRandR::SetMode: asked to change resolution for non existing mode: %s mode: %s", output.name.c_str(), mode.id.c_str());
    return false;
  }

  m_currentOutput = outputFound.name;
  m_currentMode = modeFound.id;
  char cmd[255];
  if (getenv("XBMC_BIN_HOME"))
    snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --output %s --mode %s", getenv("XBMC_BIN_HOME"), outputFound.name.c_str(), modeFound.id.c_str());
  else
    return false;
  CLog::Log(LOGINFO, "XRANDR: %s", cmd);
  int status = system(cmd);
  if (status == -1)
    return false;

  if (WEXITSTATUS(status) != 0)
    return false;

  return true;
}

XOutput CXRandR::GetCurrentOutput()
{
  Query();
  for (unsigned int j = 0; j < m_outputs.size(); j++)
  {
    if(m_outputs[j].isConnected)
      return m_outputs[j];
  }
  XOutput empty;
  return empty;
}
XMode CXRandR::GetCurrentMode(CStdString outputName)
{
  Query();
  XMode result;

  for (unsigned int j = 0; j < m_outputs.size(); j++)
  {
    if (m_outputs[j].name == outputName || outputName == "")
    {
      for (unsigned int i = 0; i < m_outputs[j].modes.size(); i++)
      {
        if (m_outputs[j].modes[i].isCurrent)
        {
          result = m_outputs[j].modes[i];
          break;
        }
      }
    }
  }

  return result;
}

void CXRandR::LoadCustomModeLinesToAllOutputs(void)
{
  Query();
  CXBMCTinyXML xmlDoc;

  if (!xmlDoc.LoadFile("special://xbmc/userdata/ModeLines.xml"))
  {
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcasecmp(pRootElement->Value(), "modelines") != 0)
  {
    // TODO ERROR
    return;
  }

  char cmd[255];
  CStdString name;
  CStdString strModeLine;

  for (TiXmlElement* modeline = pRootElement->FirstChildElement("modeline"); modeline; modeline = modeline->NextSiblingElement("modeline"))
  {
    name = modeline->Attribute("label");
    name.TrimLeft(" \n\t\r");
    name.TrimRight(" \n\t\r");
    strModeLine = modeline->FirstChild()->Value();
    strModeLine.TrimLeft(" \n\t\r");
    strModeLine.TrimRight(" \n\t\r");
    if (getenv("XBMC_BIN_HOME"))
    {
      snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --newmode \"%s\" %s > /dev/null 2>&1", getenv("XBMC_BIN_HOME"),
               name.c_str(), strModeLine.c_str());
      if (system(cmd) != 0)
        CLog::Log(LOGERROR, "Unable to create modeline \"%s\"", name.c_str());
    }

    for (unsigned int i = 0; i < m_outputs.size(); i++)
    {
      if (getenv("XBMC_BIN_HOME"))
      {
        snprintf(cmd, sizeof(cmd), "%s/xbmc-xrandr --addmode %s \"%s\"  > /dev/null 2>&1", getenv("XBMC_BIN_HOME"),
                 m_outputs[i].name.c_str(), name.c_str());
        if (system(cmd) != 0)
          CLog::Log(LOGERROR, "Unable to add modeline \"%s\"", name.c_str());
      }
    }
  }
}

/*
  int main()
  {
  CXRandR r;
  r.LoadCustomModeLinesToAllOutputs();
  }
*/
