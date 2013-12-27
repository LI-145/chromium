// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_FRAME_TREE_H_
#define CONTENT_BROWSER_FRAME_HOST_FRAME_TREE_H_

#include <string>

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/common/content_export.h"

namespace content {

class FrameTreeNode;
class Navigator;
class RenderFrameHostDelegate;
class RenderProcessHost;
class RenderViewHostDelegate;
class RenderViewHostImpl;
class RenderFrameHostManager;
class RenderWidgetHostDelegate;

// Represents the frame tree for a page. With the exception of the main frame,
// all FrameTreeNodes will be created/deleted in response to frame attach and
// detach events in the DOM.
//
// The main frame's FrameTreeNode is special in that it is reused. This allows
// it to serve as an anchor for state that needs to persist across top-level
// page navigations.
//
// TODO(ajwong): Move NavigationController ownership to the main frame
// FrameTreeNode. Possibly expose access to it from here.
//
// TODO(ajwong): Currently this class only contains FrameTreeNodes for
// subframes if the --site-per-process flag is enabled.
//
// This object is only used on the UI thread.
class CONTENT_EXPORT FrameTree {
 public:
  // Each FrameTreeNode will default to using the given |navigator| for
  // navigation tasks in the frame.
  // A set of delegates are remembered here so that we can create
  // RenderFrameHostManagers.
  // TODO(creis): This set of delegates will change as we move things to
  // Navigator.
  FrameTree(Navigator* navigator,
            RenderFrameHostDelegate* render_frame_delegate,
            RenderViewHostDelegate* render_view_delegate,
            RenderWidgetHostDelegate* render_widget_delegate,
            RenderFrameHostManager::Delegate* manager_delegate);
  ~FrameTree();

  FrameTreeNode* root() const { return root_.get(); }

  // Returns the FrameTreeNode with the given |frame_tree_node_id|.
  FrameTreeNode* FindByID(int64 frame_tree_node_id);

  // Executes |on_node| on each node in the frame tree.  If |on_node| returns
  // false, terminates the iteration immediately. Returning false is useful
  // if |on_node| is just doing a search over the tree.
  void ForEach(const base::Callback<bool(FrameTreeNode*)>& on_node) const;

  // After the FrameTree is created, or after SwapMainFrame() has been called,
  // the root node does not yet have a frame id. This is allocated by the
  // renderer and is published to the browser process on the first navigation
  // after a swap. These two functions are used to set the root node's frame
  // id.
  //
  // TODO(ajwong): Remove these once RenderFrameHost's routing id replaces
  // frame_id.
  bool IsFirstNavigationAfterSwap() const;
  void OnFirstNavigationAfterSwap(int main_frame_id);

  // Frame tree manipulation routines.
  // TODO(creis): These should take in RenderFrameHost routing IDs.
  RenderFrameHostImpl* AddFrame(int frame_routing_id,
                                int64 parent_frame_tree_node_id,
                                int64 frame_id,
                                const std::string& frame_name);
  void RemoveFrame(RenderFrameHostImpl* render_frame_host,
                   int64 parent_frame_id,
                   int64 frame_id);
  void SetFrameUrl(int64 frame_id, const GURL& url);

  // Clears process specific-state after a main frame process swap.
  // This destroys most of the frame tree but retains the root node so that
  // navigation state may be kept on it between process swaps. Used to
  // support bookkeeping for top-level navigations.
  // TODO(creis): Look into how we can remove the need for this method.
  void ResetForMainFrameSwap();

  // Convenience accessor for the main frame's RenderFrameHostImpl.
  RenderFrameHostImpl* GetMainFrame() const;

  // Allows a client to listen for frame removal.  The listener should expect
  // to receive the RenderViewHostImpl containing the frame and the renderer-
  // specific frame ID of the removed frame.
  // TODO(creis): These parameters will later change to be the RenderFrameHost.
  void SetFrameRemoveListener(
      const base::Callback<void(RenderViewHostImpl*, int64)>& on_frame_removed);

  void ClearFrameRemoveListenerForTesting();

  // Creates a RenderViewHost for a new main frame RenderFrameHost in the given
  // |site_instance|.  The RenderViewHost will have its Shutdown method called
  // when all of the RenderFrameHosts using it are deleted.
  RenderViewHostImpl* CreateRenderViewHostForMainFrame(
      SiteInstance* site_instance,
      int routing_id,
      int main_frame_routing_id,
      bool swapped_out,
      bool hidden);

  // Returns the existing RenderViewHost for a new subframe RenderFrameHost.
  // There should always be such a RenderViewHost, because the main frame
  // RenderFrameHost for each SiteInstance should be created before subframes.
  RenderViewHostImpl* GetRenderViewHostForSubFrame(SiteInstance* site_instance);

  // Keeps track of which RenderFrameHosts are using each RenderViewHost.  When
  // the number drops to zero, we call Shutdown on the RenderViewHost.
  void RegisterRenderFrameHost(RenderFrameHostImpl* render_frame_host);
  void UnregisterRenderFrameHost(RenderFrameHostImpl* render_frame_host);

  // Returns the FrameTreeNode with the given renderer-specific |frame_id|.
  // TODO(creis): Make this private and replace this with a version that takes
  // in a routing ID.
  FrameTreeNode* FindByFrameID(int64 frame_id);

 private:
  typedef std::pair<RenderViewHostImpl*, int> RenderViewHostRefCount;
  typedef base::hash_map<int, RenderViewHostRefCount> RenderViewHostMap;

  // These delegates are installed into all the RenderViewHosts and
  // RenderFrameHosts that we create.
  RenderFrameHostDelegate* render_frame_delegate_;
  RenderViewHostDelegate* render_view_delegate_;
  RenderWidgetHostDelegate* render_widget_delegate_;
  RenderFrameHostManager::Delegate* manager_delegate_;

  // Map of SiteInstance ID to a (RenderViewHost, refcount) pair.  This allows
  // us to look up the RenderViewHost for a given SiteInstance when creating
  // RenderFrameHosts, and it allows us to call Shutdown on the RenderViewHost
  // and remove it from the map when no more RenderFrameHosts are using it.
  //
  // Must be declared before |root_| so that it is deleted afterward.  Otherwise
  // the map will be cleared before we delete the RenderFrameHosts in the tree.
  RenderViewHostMap render_view_host_map_;

  scoped_ptr<FrameTreeNode> root_;

  base::Callback<void(RenderViewHostImpl*, int64)> on_frame_removed_;

  DISALLOW_COPY_AND_ASSIGN(FrameTree);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_FRAME_TREE_H_
